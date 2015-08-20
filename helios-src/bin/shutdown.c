/*
 * shutdown.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <syscall.h>

int main(int argc, char ** argv) {
	printf("Shutting Down System . . .\n");
	fflush(stdout);
	sleep(2);

	/* TODO */
	__asm__ __volatile__ ("outw %1, %0" : : "dN" ((uint16_t)0xB004), "a" ((uint16_t)0x2000));

	return 1;
}

