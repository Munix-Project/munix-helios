/*
 * mkdir.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>

int main(int argc, char ** argv) {
	if(argc < 2)
		fprintf(stderr, "%s: expected argument\n", argv[0]);
	mkdir(argv[1], 0);
	return 0;
}
