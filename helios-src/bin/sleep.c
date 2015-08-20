/*
 * sleep.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int main(int argc, char ** argv) {
	float time = atof(strdup(argv[1]));

	unsigned int seconds = (unsigned int)time;
	unsigned int subsecs = (unsigned int)((time - (float)seconds) * 100);

	return syscall_nanosleep(seconds, subsecs);
}
