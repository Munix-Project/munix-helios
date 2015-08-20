/*
 * kill.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char * argv[]) {
	int signum = SIGKILL;
	int pid = 0;

	if (argc > 2) {
		if (argv[1][0] == '-')
			signum = atoi(argv[1]+1);
		else
			return 1;

		pid = atoi(argv[2]);
	} else if (argc == 2) {
		pid = atoi(argv[1]);
	}

	if (pid)
		kill(pid, signum);

	return 0;
}

