/* This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2014 Kevin Lange
 */
/* vim: tabstop=4 shiftwidth=4 noexpandtab
 *
 * init
 *
 * Provides the standard boot routines and
 * calls the user session (compositor / terminal)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <syscall.h>
#include <sys/wait.h>

#define OS_HOSTNAME "helios"

void set_console(void) {
	int _stdin  = open("/dev/null", O_RDONLY);
	int _stdout = open("/dev/ttyS0", O_WRONLY);
	int _stderr = open("/dev/ttyS0", O_WRONLY);

	if (_stdout < 0) {
		_stdout = open("/dev/null", O_WRONLY);
		_stderr = open("/dev/null", O_WRONLY);
	}
}

void set_hostname(void){
	FILE* f_hostname = fopen("/etc/hostname","r");
	if(!f_hostname)
		syscall_sethostname(OS_HOSTNAME);
	else {
		char buff[256];
		fgets(buff, 255, f_hostname);
		if(buff[strlen(buff)-1] == '\n')
			buff[strlen(buff)-1] = '\0';
		syscall_sethostname(buff);
		setenv("HOST", buff, 1);
		fclose(f_hostname);
	}
}

void setupOS(void){
	set_console();
	set_hostname();
}

void run(char * args[]){
	int pid = fork();
	if(!pid) {
		execvp(args[0], args);
		exit(0);
	} else {
		int pid = 0;
		do
			pid = wait(NULL);
		while((pid > 0) || (pid == -1 && errno == EINTR));
	}
}

int main(int argc, char * argv[]) {
	/* Prepare OS, which include configs and initializations */
	setupOS();

	/* All is configured for startup, launch the terminal / GUI */
#if 1 /* TODO: Remove this macro after testing with GUI is done */
	run((char *[]){"/usr/bin/start", NULL});
#else
	run((char*[]){"/bin/terminal", "-l", NULL});
#endif
}
