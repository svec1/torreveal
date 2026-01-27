#include <getopt.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

#include "common.h"

#define TORNET "tornet"
#define TOR_PATH "/usr/bin/tor"
#define MS_TO_NS 1000000
#define TRIVIAL_LATENCY 10 * MS_TO_NS

atomic_bool working;
atomic_bool to_run_tor;
atomic_bool to_reload_tor;

enum tor_management_error { failed_to_run = 1, failed_to_kill, failed_to_reload };

void sig_handler(int signal) {
	atomic_store(&working, 0);
	thrd_sleep(&(struct timespec) { .tv_nsec = TRIVIAL_LATENCY * 10 }, NULL);
	_log("Exit in progress...");
	_exit(0);
}

int get_ip(char *buffer, char *const proxy) {
	size_t ret;
	const char *const url = "https://api.ipify.org/";

	if ((ret = curl_request(url, proxy, curl_write_callback_impl, buffer))) {
		_log("Failed to get response(%s): %s", url, get_curl_error(ret));
		return 0;
	}
	return 1;
}

int manage_tor() {
	enum tor_management_error ret = 0;

	char *start_tor[]  = { "systemctl", "start", "tor", NULL };
	char *stop_tor[]   = { "systemctl", "stop", "tor", NULL };
	char *reload_tor[] = { "systemctl", "reload", "tor", NULL };

	while (atomic_load(&working)) {
		thrd_sleep(&(struct timespec) { .tv_nsec = TRIVIAL_LATENCY }, NULL);
		if (atomic_load(&to_run_tor) || !check_running("tor")) {
			int ret_call = wait_process(spawn_pure_process(start_tor));
			if (ret_call != 0) {
				ret = failed_to_run;
				break;
			}

			atomic_store(&to_run_tor, 0);
		} else if (atomic_load(&to_reload_tor)) {
			int ret_call = wait_process(spawn_pure_process(reload_tor));
			if (ret_call != 0) {
				ret = failed_to_reload;
				break;
			}
			atomic_store(&to_reload_tor, 0);

			char buffer[16];
			if (get_ip(buffer, "socks5://localhost:9050/"))
				_log("Was changed ip: %s", buffer);
		}
	}

	atomic_store(&working, 0);
	wait_process(spawn_pure_process(stop_tor));

	return ret;
}

void on_killswitch() {
	system("iptables -N TORNET-KILLSWITCH");
	system("iptables -A TORNET-KILLSWITCH -d 127.0.0.1/8 -j ACCEPT");
	system("iptables -A TORNET-KILLSWITCH -d 192.168.0.0/16 -j ACCEPT");
	system("iptables -A TORNET-KILLSWITCH -d 172.16.0.0/12 -j ACCEPT");
	system("iptables -A TORNET-KILLSWITCH -d 10.0.0.0/8 -j ACCEPT");
	system("iptables -A TORNET-KILLSWITCH -p tcp --dport 9050 -j ACCEPT");
	system("iptables -A TORNET-KILLSWITCH -j DROP");
	system("iptables -A OUTPUT -j TORNET-KILLSWITCH");
}

void off_killswitch() {
	system("iptables -D OUTPUT TORNET-KILLSWITCH");
	system("iptables -F TORNET-KILLSWITCH");
	system("iptables -X TORNET-KILLSWITCH");
}

int main(int argc, char **argv) {
	init_log();
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	printf("%s", get_logo());

	atomic_init(&working, 1);
	atomic_init(&to_run_tor, 1);
	atomic_init(&to_reload_tor, 0);

	char ip[16];
	char *detach[] = { "self", NULL };
	int count      = -1;
	int delay      = 15;
	int on_ks      = 0;
	int pid_tmp    = 0;

	int opt;
	while ((opt = getopt(argc, argv, ":i:c:dks")) != -1) {
		switch (opt) {
			case 'i':
				delay = atoi(optarg);
				if (delay < 1)
					exit_with_error("The delay cannot be less than 1.");

				break;
			case 'c':
				if (delay < 0)
					exit_with_error("The number of reloads cannot be less than 0.");

				count = atoi(optarg);
				break;
			case 'd':
				spawn_pure_process(detach);
				return 0;
			case 'k':
				pid_tmp = check_running(TORNET);
				kill_process(pid_tmp);
				break;
			case 's':
				on_ks = 1;
				break;
			case '?':
				exit_with_error("Unknown option: %c", optopt);
		}
	}

	if (on_ks)
		on_killswitch();

	get_ip(ip, "");

	thrd_t th_handle;
	if (thrd_create(&th_handle, manage_tor, NULL))
		exit_with_error("Cannot start manager of the tor process.");

	_log("Tor running: %i", check_running("tor"));
	_log("Your own ip: %s", ip);
	while (count && atomic_load(&working)) {
		thrd_sleep(&(struct timespec) { .tv_sec = delay }, NULL);
		atomic_store(&to_reload_tor, 1);
		if (count != -1)
			--count;
	}

	int ret;
	thrd_join(th_handle, &ret);

	switch (ret) {
		case failed_to_run:
			exit_with_error("Failed to run tor process.");
			break;
		case failed_to_kill:
			exit_with_error("Cannot kill the tor process.");
			break;
	}

	if (on_ks)
		off_killswitch();

	exit(0);
}
