/*
 * readlink.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINK_SIZE 4096

int main(int argc, char * argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [link]\n", argv[0]);
		return 1;
	}

	char buf[MAX_LINK_SIZE];
	if (readlink(argv[1], buf, sizeof(buf)) < 0) {
		perror("link");
		return 1;
	}
	fprintf(stdout, "%s\n", buf);

	return 0;
}
