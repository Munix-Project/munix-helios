/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
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
#include <security/helios_auth.h>

#define LINE_LEN 1024

static uint8_t already_saw_motd = 0;
uint32_t child = 0;
uint32_t shellno = -1;

void sig_pass(int sig) {
	/* Pass onto the shell */
	if (child)
		kill(child, sig);
	/* Else, ignore */
}

void sig_segv(int sig) {
	printf("Segmentation fault.\n");
	exit(127 + sig);
}

uint8_t multishell_cont = 0;

void sig_tstp(int sig) {
	multishell_cont = 0;
	if (child) kill(child, sig);
	while(!multishell_cont) usleep(100); /*wait for SIGCONT*/
}

void sig_cont(int sig) {
	multishell_cont = 1;
}

int main(int argc, char ** argv) {
	signal(SIGINT, sig_pass);
	signal(SIGWINCH, sig_pass);
	signal(SIGSEGV, sig_segv);
	signal(SIGTSTP, sig_tstp);
	signal(SIGCONT, sig_cont);

	if(argc>1) {
		int c;
		while((c = getopt(argc, argv, "q:")) != -1)
			switch(c){
			case 'q':
				shellno = atoi(argv[1]);
				/* Wait for the multishell monitor to allow this shellno to continue */
				sig_tstp(SIGCONT);
				break;
			}
	} else {
		system("uname -a");
		system("cat /etc/intro");
		fflush(stdout);
		printf("\n");
	}

	while (1) {
		char * username = malloc(sizeof(char) * 1024);
		char * password = malloc(sizeof(char) * 1024);

		char _hostname[256];
		syscall_gethostname(_hostname);

		/* Ask for username */
		printf("\e[47;30m%s login:\e[0m ", _hostname);
		fflush(stdout);
		char * r = fgets(username, 1024, stdin);
		if (!r) {
			clearerr(stdin);
			fprintf(stderr, "\n");
			fprintf(stderr, "\nLogin failed.\n");
			continue;
		}
		username[strlen(username)-1] = '\0';

		/* Ask for password */
		printf("\e[47;30m  password:  \e[0m ");
		fflush(stdout);
		/* Disable echo */
		struct termios old, new;
		tcgetattr(fileno(stdin), &old);
		new = old;
		new.c_lflag &= (~ECHO);
		tcsetattr(fileno(stdin), TCSAFLUSH, &new);

		r = fgets(password, 1024, stdin);
		if (!r) {
			clearerr(stdin);
			tcsetattr(fileno(stdin), TCSAFLUSH, &old);
			fprintf(stderr, "\n");
			fprintf(stderr, "\nLogin failed.\n");
			continue;
		}
		password[strlen(password)-1] = '\0';
		tcsetattr(fileno(stdin), TCSAFLUSH, &old);
		fprintf(stdout, "\n");

		/* Done */

		int uid = helios_auth(username, password);
		if (uid < 0) {
			fprintf(stdout, "\nLogin failed.\n");
			continue;
		}

		fprintf(stdout, "\e[32;1m\nLogin Successful!");
		if(!already_saw_motd){
			fprintf(stdout, "\e[37;1;40m\n\n--------------------------------------------------------------------------------");
			fflush(stdout);
			system("cat /etc/motd");
			fprintf(stdout,"\n"); fflush(stdout);
			already_saw_motd = 1;
		} else fprintf(stdout, "\n\n");
		fprintf(stdout,"\e[0m");
		fflush(stdout);

		pid_t pid = getpid();
		pid_t f = fork();
		if (getpid() != pid) {
			setuid(uid);
			helios_auth_set_vars();
			char * args[] = {
				getenv("SHELL"),
				NULL
			};
			execvp(args[0], args);
		} else {
			child = f;
			int result;
			do {
				result = waitpid(f, NULL, 0);
			} while (result < 0);
		}
		child = 0;
		free(username);
		free(password);
	}

	return 0;
}
