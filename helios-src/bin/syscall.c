/*
 * syscall.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	if (argc < 2) {
		fprintf(stderr, "%s: usage: [callno]\n", argv[0]);
		return 1;
	}
	return syscall_system_function(atoi(argv[1]), &argv[2]);
}
