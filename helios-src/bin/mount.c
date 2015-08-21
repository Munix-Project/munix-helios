/*
 * mount.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */
#include <stdio.h>

int main(int argc, char ** argv) {
	if(argc < 4) {
		fprintf(stderr, "Usage: %s type device mountpoint\n", argv[0]);
		return 1;
	}

	if (mount(argv[2], argv[3], argv[1], 0, NULL) < 0) {
		perror("mount");
		return 1;
	}

	return 0;
}
