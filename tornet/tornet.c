#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

#include "common.h"

#define TOR_PATH "/usr/bin/tor"

#define MS_TO_NS 1000000

atomic_bool working;
atomic_bool to_run_tor;
atomic_bool to_reload_tor;

enum tor_management_error { failed_to_run = 1, failed_to_kill, failed_to_reload };

int spawn_process(char *const argv[]) {
	int pid;
	if ((pid = fork()) == -1)
		return -1;
	else if (!pid) {
		if (execvp(argv[0], argv))
			exit_with_error("Failed to execute proccess");
	}

	return pid;
}

int wait_process(int pid) {
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

int manage_tor() {
	char *start_tor[]  = { "systemctl", "start", "tor", NULL };
	char *stop_tor[]   = { "systemctl", "stop", "tor", NULL };
	char *reload_tor[] = { "systemctl", "reload", "tor", NULL };

	enum tor_management_error ret = 0;

	while (atomic_load(&working)) {
		thrd_sleep(&(struct timespec) { .tv_nsec = 10 * MS_TO_NS }, NULL);
		if (atomic_load(&to_run_tor)) {
			int ret_call = wait_process(spawn_process(start_tor));
			if (ret_call != 0) {
				ret = failed_to_run;
				break;
			}

			atomic_store(&to_run_tor, 0);
		} else if (atomic_load(&to_reload_tor)) {
			int ret_call = wait_process(spawn_process(reload_tor));
			if (ret_call != 0) {
				ret = failed_to_reload;
				break;
			}
			atomic_store(&to_reload_tor, 0);
			_log("Was changed ip.");
		}
	}

	atomic_store(&working, 0);
	wait_process(spawn_process(stop_tor));

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

	atomic_init(&working, 1);
	atomic_init(&to_run_tor, 1);
	atomic_init(&to_reload_tor, 0);

	thrd_t th_handle;
	if (thrd_create(&th_handle, manage_tor, NULL))
		exit_with_error("Cannot start manager of the tor process.");

	int count = -1;
	int delay = 1;

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

	_exit(0);
}
