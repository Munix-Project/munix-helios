/* This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2014 Kevin Lange
 */
/* vim:tabstop=4 shiftwidth=4 noexpandtab
 *
 * mkdir
 *
 * Create a directory.
 */

#include <stdio.h>

int main(int argc, char ** argv) {
	if(argc < 2)
		fprintf(stderr, "%s: expected argument\n", argv[0]);
	mkdir(argv[1], 0);
	return 0;
}
