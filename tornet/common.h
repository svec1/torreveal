#ifndef COMMON_H
#define COMMON_H

void init_log();
void dump_log();
void _exit(int);
void exit_with_error(const char *str);
void _log(const char *str);
void _logd(const char *str);

#endif
