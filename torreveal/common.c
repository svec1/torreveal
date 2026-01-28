#include <curl/curl.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

const char *logname;
const char *logfile_name;
FILE *logfile = NULL;

void init_log(const char *_logname, const char *_logfile_name) {
	logname      = _logname;
	logfile_name = _logfile_name;
	logfile      = fopen(logfile_name, "a");
}
void dump_log() {
	if (logfile)
		fclose(logfile);
}

void _exit(int ret) {
	dump_log();
	exit(ret);
}

void exit_with_error(const char *format, ...) {
	va_list vl;

	printf(RED_COLOR);

	va_start(vl, format);
	vprintf(format, vl);
	va_end(vl);

	printf("\033[0m\n");

	_exit(1);
}

void _log(const char *format, ...) {
	if (!logfile)
		return;

	va_list vl;

	va_start(vl, format);
	printf("\r[%s]: ", logname);
	vprintf(format, vl);
	va_end(vl);

	printf(END_COLOR "\n");

	va_start(vl, format);
	vfprintf(logfile, format, vl);
	va_end(vl);

	fwrite("\n", sizeof(char), 1, logfile);
}
void _logd(const char *format, ...) {
#ifdef DEBUG
	_log(format);
#endif
}

int spawn_process(char *const argv[], int newfd, int fd) {
	int pid;
	if ((pid = fork()) == -1)
		return -1;
	else if (!pid && strcmp(argv[0], "self")) {
		if (newfd != -1) {
			dup2(newfd, fd);
			close(newfd);
		}

		if (execvp(argv[0], argv))
			exit_with_error("Failed to execute proccess");
	}

	return pid;
}

int spawn_pure_process(char *const argv[]) {
	return spawn_process(argv, -1, 0);
}

int wait_process(int pid) {
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}
int kill_process(int pid) {
	return kill(pid, SIGTERM);
}

int check_running(char *program) {
	int pipefd[2];
	if (pipe(pipefd) == -1)
		exit_with_error("Failed to init pipe.");

	int pid = 0;
	char output[16];
	char *run_grep[] = { "pgrep", "-f", program, NULL };

	spawn_process(run_grep, pipefd[1], STDOUT_FILENO);
	close(pipefd[1]);

	output[0] = '\0';
	if (read(pipefd[0], &output, 16) < 0)
		exit_with_error("Failed to check %s.", program);
	close(pipefd[0]);

	if (!output[0])
		return 0;

	for (int i = 0; i < 16 && output[i] != '\n' && output[i] != '\0'; ++i)
		pid = pid * 10 + (output[i] - '0');

	return pid;
}

CURL_REMOTE_TYPE curl_init(const char *const proxy, size_t (*write_callback)(char *, size_t, size_t, void *)) {
	CURL *handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(handle, CURLOPT_DIRLISTONLY, 1L);
	curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 1L);
	curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(handle, CURLOPT_SSLVERSION, (long) CURL_SSLVERSION_TLSv1_2);

	curl_easy_setopt(handle, CURLOPT_PROXY, proxy);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);

	return handle;
}

void curl_dump(CURL_REMOTE_TYPE handle) {
	if (!handle)
		return;
	curl_easy_cleanup(handle);
}
void curl_change_proxy(CURL_REMOTE_TYPE handle, const char *const proxy) {
	if (!handle)
		exit_with_error("Curl is not initialized.");
	curl_easy_setopt(handle, CURLOPT_PROXY, proxy);
}
size_t curl_url_request(CURL_REMOTE_TYPE handle, const char *const url, void *user_data, int reset_last_connection) {
	if (!handle)
		exit_with_error("Curl is not initialized.");

	CURLcode ret;

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_FRESH_CONNECT, (long) reset_last_connection);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, user_data);

	ret = curl_easy_perform(handle);

	return ret;
}
size_t curl_write_callback_impl(char *data, size_t size, size_t nmemb, void *userdata) {
	if (!userdata)
		exit_with_error("Failed to write response in buffer(userdata).");

	memset(userdata, '\0', nmemb + 1);
	memcpy(userdata, data, nmemb);
	return nmemb;
}

const char *get_curl_error(size_t error) {
	return curl_easy_strerror(error);
}

