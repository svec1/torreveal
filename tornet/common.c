#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define TORNET_LOG_NAME "TORNET"
#define TORNET_LOGFILE_NAME "tn.log"

FILE *logfile;

void init_log() {
	logfile = fopen(TORNET_LOGFILE_NAME, "a");
}
void dump_log() {
	fclose(logfile);
}

void _exit(int ret) {
	dump_log();
	exit(ret);
}

void exit_with_error(const char *str) {
	printf("%s\n", str);
	_exit(1);
}

void _log(const char *str) {
	if (!logfile)
		exit_with_error("Log file is not initialized.");

	size_t len = strlen(str), ret = fwrite((void *) str, sizeof(char), len, logfile);

	printf("[%s]: %s\n", TORNET_LOG_NAME, str);
	if (ret < len)
		exit_with_error("Failed to write log.");
	fwrite("\n", sizeof(char), 1, logfile);
}
void _logd(const char *str) {
#ifdef DEBUG
	_log(str);
#endif
}
