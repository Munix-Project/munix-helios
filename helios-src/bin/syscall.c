/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
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
