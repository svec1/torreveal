#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>

const char *get_logo();

void init_log();
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

size_t curl_request(const char *const url, const char *const proxy,
					size_t (*write_callback)(char *, size_t, size_t, void *), void *userdata);
size_t curl_write_callback_impl(char *data, size_t size, size_t nmemb, void *userdata);
const char *get_curl_error(size_t error);


#endif
