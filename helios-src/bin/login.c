/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2014 Kevin Lange
 *
 * Login Service
 *
 * Provides the user with a login prompt and starts their session.
 *
 */

#include <stdio.h>
#include <stddef.h>
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
#include <spinlock.h>
#include <shmon.h>

extern int fileno(FILE *); /* eclipse can't find this declaration on stdio.h, so we put this here */

#define LINE_LEN 1024

extern char shm_key[30];

static uint8_t already_saw_motd = 0;
char * passed_username = NULL;
uint32_t child = 0;
uint8_t multishell_cont = 0;
uint8_t just_exited = 0;

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

void sig_tstp(int sig) {
	multishell_cont = 0;
	if (child) kill(child, sig);

	while(!multishell_cont){
		if(listen_to_shm()) break;
		usleep(100); /*wait for SIGCONT and reap flag*/
	}
}

void sig_cont(int sig) {
	multishell_cont = 1;
}

void sig_kill(int sig) {
	sig_pass(sig);
	exit(EXIT_FAILURE);
}

void show_intro() {
	system("uname -a");
	system("cat /etc/intro");
	fflush(stdout);
	printf("\n");
}

int main(int argc, char ** argv) {
	signal(SIGINT, 	 sig_pass);
	signal(SIGWINCH, sig_pass);
	signal(SIGSEGV,  sig_segv);
	signal(SIGTSTP,  sig_tstp);
	signal(SIGCONT,  sig_cont);
	signal(SIGKILL,  sig_kill);

	init_shm();

	if(argc > 1) {
		int c;
		while((c = getopt(argc, argv, "u:qi")) != -1)
			switch(c){
			case 'q':
				/* Wait for the multishell monitor to allow this shellno to continue */
				sig_tstp(SIGCONT);
				break;
			case 'u': passed_username = optarg; break;
			case 'i': already_saw_motd = 1; break;
			}
	} else {
		show_intro();
	}

	while (1) {
		char * username = malloc(sizeof(char) * 1024);
		char * password = malloc(sizeof(char) * 1024);

		char _hostname[256];
		syscall_gethostname(_hostname);

		/* Ask for username */
		char * r;
		if(just_exited) {
			printf("\e[47;30m%s login:\e[0m ", _hostname);
			fflush(stdout);
			r = fgets(username, 1024, stdin);
			if (!r) {
				clearerr(stdin);
				fprintf(stderr, "\n");
				fprintf(stderr, "\nLogin failed.\n");
				continue;
			}
			username[strlen(username)-1] = '\0';
		} else {
			strcpy(username, passed_username);
		}

		/* Ask for password */
		char for_msg[30];
		sprintf(for_msg, " for %s", username);
		printf("\e[47;30m  password%s:  \e[0m ", passed_username != NULL ? for_msg : "");
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

		/* Check first if user is already logged in on another process */
		int switch_pid = -1;
		if(already_saw_motd && (switch_pid = shmon_get_user(username)) != -1 && switch_pid != getpid()) {
			/* If he is, switch immediately to the pid where he lives in and then reap this login process */
			/* Use shared memory instead of signals due to permission problems */
			send_kill_msg_login(switch_pid);

			/* KILL THIS SHELL AND LOGIN */
			sig_pass(SIGKILL);
			return 0;
			_exit(EXIT_SUCCESS);
		}
		/* Else continue */

		fprintf(stdout, "\e[32;1m\nLogin Successful!");

		/* Send command to shmon to add this user to the multishell_session hashmap */
		shmon_send_user(username);

		if(!already_saw_motd){
			already_saw_motd = 1;
			fprintf(stdout,"\n");
			fflush(stdout);
			system("cat /etc/motd");
			fprintf(stdout,"\n");
			fflush(stdout);
		} else fprintf(stdout, "\n\n");
		fprintf(stdout, "\e[0m");
		fflush(stdout);

		pid_t pid = getpid();
		pid_t f = fork();
		if (getpid() != pid) {
			setuid(uid);
			helios_auth_set_vars();
			char thispid_str[5];
			sprintf(thispid_str, "%d", pid);
			char * args[] = {
				getenv("SHELL"),
				"-p",
				thispid_str,
				NULL
			};
			execvp(args[0], args);
		} else {
			child = f;
			int result;
			do
				result = waitpid(f, NULL, 0);
			while (result < 0);
			just_exited = 1;
		}
		child = 0;
		free(username);
		free(password);
	}
	syscall_shm_release(shm_key);
	return 0;
}
