#include <stdarg.h>
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

void exit_with_error(const char *format, ...) {
	va_list vl;

	va_start(vl, format);
	vprintf(format, vl);
	va_end(vl);

	printf("\n");

	_exit(1);
}

void _log(const char *format, ...) {
	if (!logfile)
		exit_with_error("Log file is not initialized.");

	va_list vl;

	va_start(vl, format);
	printf("[%s]: ", TORNET_LOG_NAME);
	vprintf(format, vl);
	va_end(vl);

	printf("\n");

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
const char *get_logo() {
	return "+-------------------------------------------------------+ \n"
		   "|  ████████╗ ██████╗ ██████╗ ███╗   ██╗███████╗████████╗| \n"
		   "|  ╚══██╔══╝██╔═══██╗██╔══██╗████╗  ██║██╔════╝╚══██╔══╝| \n"
		   "|     ██║   ██║   ██║██████╔╝██╔██╗ ██║█████╗     ██║   | \n"
		   "|     ██║   ██║   ██║██╔══██╗██║╚██╗██║██╔══╝     ██║   | \n"
		   "|     ██║   ╚██████╔╝██║  ██║██║ ╚████║███████╗   ██║   | \n"
		   "|     ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝   ╚═╝   | \n"
		   "+-------------------------------------------------------+ \n";
}
