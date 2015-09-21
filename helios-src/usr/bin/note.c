/* This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2014 Kevin Lange
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int spkr = 0;

struct spkr {
	int length;
	int frequency;
};

void note(int length, int frequency) {
	struct spkr s = {
		.length = length,
		.frequency = frequency,
	};

	write(spkr, &s, sizeof(s));
}

int main(int argc, char * argv[]) {
	spkr = open("/dev/spkr", O_WRONLY);
	if (spkr == -1) {
		fprintf(stderr, "%s: could not open speaker\n", argv[0]);
	}

	if(argc > 1) {
		if(argc > 2) {
			/* Frequency AND Length: */
			note(atoi(argv[2]), atoi(argv[1]));
		} else {
			/* Just Frequency: */
			note(1, atoi(argv[1]));
		}
	}else
		note(1, 1);

	return 0;
}
