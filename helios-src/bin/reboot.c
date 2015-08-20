/*
 * reboot.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <syscall.h>

int main(int argc, char ** argv) {
	printf("Rebooting System . . .\n");
	fflush(stdout);
	sleep(2);

	if(syscall_reboot() < 0) {
		printf("%s: permission denied\n", argv[0]);
		fflush(stdout);
	}
	return 1;
}
