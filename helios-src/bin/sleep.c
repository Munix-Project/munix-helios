/* This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013 Kevin Lange
 */
/*
 * Sleep
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
