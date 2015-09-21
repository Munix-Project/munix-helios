/* This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013 Kevin Lange
 */
/* vim:tabstop=4 shiftwidth=4 noexpandtab
 *
 * prints "Hello World!" to standard out.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, int * argv[]) {
	printf("\e[32mHello friend!\e[0m\n"); fflush(stdout);
	return 0;
}

