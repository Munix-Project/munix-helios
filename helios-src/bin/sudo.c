/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2014 Kevin Lange
 */

#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <security/helios_auth.h>

int main(int argc, char ** argv) {
	int fails = 0;

	if (argc < 2) {
		fprintf(stderr, "usage: %s [command]\n", argv[0]);
		return 1;
	}

	while (1) {
		char * username = getenv("USER");
		char * password = malloc(sizeof(char) * 1024);

		fprintf(stdout, "[%s] password for %s: ", argv[0], username);
		fflush(stdout);

		/* Disable echo */
		struct termios old, new;
		tcgetattr(fileno(stdin), &old);
		new = old;
		new.c_lflag &= (~ECHO);
		tcsetattr(fileno(stdin), TCSAFLUSH, &new);

		/* Ask for password: */
		fgets(password, 1024, stdin);
		password[strlen(password)-1] = '\0';
		tcsetattr(fileno(stdin), TCSAFLUSH, &old);
		fprintf(stdout, "\n");

		int uid = helios_auth(username, password);
		if (uid < 0) {
			if (++fails == 3) {
				fprintf(stderr, "%s: %d incorrect password attempts\n", argv[0], fails);
				break;
			}
			fprintf(stderr, "Sorry, try again.\n");
			continue;
		}

		char ** args = &argv[1];
		int ret = execvp(args[0], args);
		fprintf(stderr, "%s: %s (%d): command not found\n", argv[0], args[0], ret);
		return 1;
	}
	return 1;
}
