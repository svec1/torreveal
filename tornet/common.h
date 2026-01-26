#ifndef COMMON_H
#define COMMON_H

void init_log();
void dump_log();
void _exit(int);
void exit_with_error(const char *format, ...);
void _log(const char *format, ...);
void _logd(const char *format, ...);
const char *get_logo();

#endif
