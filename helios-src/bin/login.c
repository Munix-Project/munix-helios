/*
 * login.c
 *
 *  Created on: Aug 17, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdint.h>
#include <syscall.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/utsname.h>

uint32_t pid_child = 0;

void sig_into_shell(int sig) {
	if(pid_child)
		kill(pid_child, sig);
}

void sig_segv(int sig) {
	printf("Segmentation fault.\n");
	exit(sig + 127);
	/*Shit went down son*/
}

int main(int argc, char * argv[]) {
	/* First print system info: */
	printf("\n");
	system("uname -a");
	printf("\n");

	signal(SIGINT, sig_into_shell);
	signal(SIGWINCH, sig_into_shell);
	signal(SIGSEGV, sig_segv);

	/* Entering login screen: */
	while(1) {
		char * usr = malloc(sizeof(char) * 1024);
		char * pass = malloc(sizeof(char) * 1024);

		char host[256];
		syscall_gethostname(host);

		/* Ask for username: */
		fprintf(stdout, "@ %s - login: ", host); fflush(stdout);
		char * ret = fgets(usr, 1024, stdin);
		if(!ret) {
			clearerr(stdin);
			fprintf(stderr,"\n");
			fprintf(stderr, "\nLogin Failed.\n");
			continue;
		}
		usr[strlen(usr) - 1] = '\0';

		/* Ask for password: */
		fprintf(stdout, "password: "); fflush(stdout);
	}

	return 0;
}
