/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
 */

#include <stdio.h>

int main(int argc, char ** argv) {
	unsigned int nulls = 0;
	for (unsigned int i = 0;; i++) {
		if (!argv[i]) {
			if (++nulls == 2)
				break;
			else continue;
		}
		if (nulls == 1)
			printf("%s\n", argv[i]);
	}
}
