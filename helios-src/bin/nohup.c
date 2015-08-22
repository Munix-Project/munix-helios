/*
 * nohup.c
 *
 *  Created on: Aug 22, 2015
 *      Author: miguel
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <process.h>
#include <string.h>

int main(int argc, char ** argv) {
	if(argc > 1){
		int pid = fork();
		if(!pid) {
			char cmd[256];
			memset(cmd, 0, 256);
			for(int i=1;i<argc;i++){
				strcat(cmd, argv[i]);
				strcat(cmd, " ");
			}
			system(cmd);
		}
	} else {
		fprintf(stderr, "%s: missing operand\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	return 0;
}
