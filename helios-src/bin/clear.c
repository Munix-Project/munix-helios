/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
 */

#include <stdio.h>

int main(int argc, char ** argv) {
	printf("\e[0;0H\e[2J");
	fflush(stdout);
	return 0;
}

