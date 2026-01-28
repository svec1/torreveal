#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

#define RED_COLOR "\033[31m"
#define GREEN_COLOR "\033[32m"
#define BRIGHT_BLACK_COLOR "\033[90m"

#define END_COLOR "\033[0m"

typedef void *CURL_REMOTE_TYPE;

void init_log(const char *_logname, const char *_logfile_name);
void dump_log();
void _exit(int);
void exit_with_error(const char *format, ...);
void _log(const char *format, ...);
void _logd(const char *format, ...);

int spawn_process(char *const argv[], int newfd, int fd);
int spawn_pure_process(char *const argv[]);
int wait_process(int pid);
int kill_process(int pid);
int check_running(char *program);

CURL_REMOTE_TYPE curl_init(const char *const proxy, size_t (*write_callback)(char *, size_t, size_t, void *));
void curl_dump(CURL_REMOTE_TYPE handle);
void curl_change_proxy(CURL_REMOTE_TYPE handle, const char *const proxy);
size_t curl_url_request(CURL_REMOTE_TYPE handle, const char *const url, void *userdata, int reset_last_connection);
size_t curl_write_callback_impl(char *data, size_t size, size_t nmemb, void *userdata);
const char *get_curl_error(size_t error);


#endif
