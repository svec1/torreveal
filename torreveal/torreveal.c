#include <getopt.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

#include "common.h"

#define TORREVEAL "torreveal"
#define TORREVEAL_LOGFILE_NAME "trr.log"
#define TOR_PATH "/usr/bin/tor"
#define MS_TO_NS 1000000
#define TRIVIAL_LATENCY 100 * MS_TO_NS

atomic_bool working;
atomic_bool to_run_tor;
atomic_bool to_reload_tor;

CURL_REMOTE_TYPE curl_handle;

enum tor_management_error { failed_to_run = 1, failed_to_reload };

const char *get_logo() {
	return " _____ ___  ____       ____  _______     _______    _    _     \n"
		   "|_   _/ _ \\|  _ \\     |  _ \\| ____\\ \\   / / ____|  / \\  | |    \n"
		   "  | || | | | |_) |____| |_) |  _|  \\ \\ / /|  _|   / _ \\ | |    \n"
		   "  | || |_| |  _ <_____|  _ <| |___  \\   / | |___ / ___ \\| |___ \n"
		   "  |_| \\___/|_| \\_\\    |_| \\_\\_____|  \\_/  |_____/_/   \\_\\_____|\n";
}

void thread_sleep_ms(size_t interval) {
	thrd_sleep(&(struct timespec) { .tv_nsec = interval }, NULL);
}
void thread_sleep_s(size_t interval) {
	thrd_sleep(&(struct timespec) { .tv_sec = interval }, NULL);
}

void atomic_bool_wait(atomic_bool *atomic_var, _Bool until_value) {
	while (atomic_load(atomic_var) == until_value)
		thread_sleep_ms(TRIVIAL_LATENCY);
}

void sig_handler(int signal) {
	curl_dump(curl_handle);
	atomic_store(&working, 0);
	thread_sleep_ms(TRIVIAL_LATENCY);
	_log("Exit in progress...");
	_exit(0);
}

int get_ip(char *buffer) {
	const char *const url = "https://api.ipify.org/";
	size_t ret;

	if ((ret = curl_url_request(curl_handle, url, buffer, 1))) {
		_log(RED_COLOR "Failed to get response(%s): %s", url, get_curl_error(ret));
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
		if (atomic_load(&to_run_tor) || !check_running(TOR_PATH)) {
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
		}
		thread_sleep_ms(TRIVIAL_LATENCY);
	}

	atomic_store(&working, 0);
	wait_process(spawn_pure_process(stop_tor));

	return ret;
}

int main(int argc, char **argv) {
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	atomic_init(&working, 1);
	atomic_init(&to_run_tor, 1);
	atomic_init(&to_reload_tor, 0);

	char ip[16];
	char *detach[] = { "self", NULL };
	int on_log     = 1;
	int show_name  = 0;
	int count      = -1;
	int interval   = 15;
	int pid_tmp    = 0;

	int opt;
	while ((opt = getopt(argc, argv, ":lni:c:dks")) != -1) {
		switch (opt) {
			case 'l':
				on_log = 0;
				break;
			case 'n':
				show_name = 1;
				break;
			case 'i':
				interval = atoi(optarg);
				if (interval < 1)
					exit_with_error("The delay cannot be less than 1.");

				break;
			case 'c':
				count = atoi(optarg);
				if (count < 0)
					exit_with_error("The number of reloads cannot be less than 0.");

				break;
			case 'd':
				if (spawn_pure_process(detach))
					_exit(0);
				break;
			case 'k':
				pid_tmp = check_running(TORREVEAL);
				kill_process(pid_tmp);
				_exit(0);
			case 's':
				curl_change_proxy(curl_handle, "socks5://localhost:9050/");
				if (!get_ip(ip))
					_exit(1);
				_log(BRIGHT_BLACK_COLOR "Your ip address on tor: %s", ip);
				_exit(0);
			case '?':
				exit_with_error("Unknown option: %c", optopt);
		}
	}
	if (on_log)
		init_log(TORREVEAL, TORREVEAL_LOGFILE_NAME);
	if (show_name)
		printf("\r%s\n", get_logo());
	curl_handle = curl_init("", curl_write_callback_impl);

	if (!get_ip(ip))
		_exit(1);

	curl_change_proxy(curl_handle, "socks5://localhost:9050/");

	thrd_t th_handle;
	if (thrd_create(&th_handle, manage_tor, NULL))
		exit_with_error("Cannot start manager of the tor process.");

	atomic_bool_wait(&to_run_tor, 1);
	pid_tmp = check_running(TOR_PATH);
	if (!pid_tmp) {
		_log(RED_COLOR "Failed to run a tor");
		_exit(1);
	}

	_log(GREEN_COLOR "Tor running: %i", pid_tmp);
	_log(BRIGHT_BLACK_COLOR "The ip address will change every: %is", interval);
	_log(BRIGHT_BLACK_COLOR "Your own ip address: %s", ip);
	while (count && atomic_load(&working)) {
		thread_sleep_s(interval);
		atomic_store(&to_reload_tor, 1);
		atomic_bool_wait(&to_reload_tor, 1);

		if (get_ip(ip))
			_log(BRIGHT_BLACK_COLOR "Was changed ip: %s", ip);
		if (count != -1)
			--count;
	}
	atomic_store(&working, 0);

	int ret;
	thrd_join(th_handle, &ret);
	curl_dump(curl_handle);

	switch (ret) {
		case failed_to_run:
			exit_with_error("Failed to run tor process.");
			break;
		case failed_to_reload:
			exit_with_error("Cannot reload the tor process.");
			break;
	}

	_exit(0);
}
