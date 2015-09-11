/*
 * micro.c
 *
 *  Created on: Aug 30, 2015
 *      Author: miguel
 *
 *  A text editor for the Helios Operating System
 */

#include <stdio.h>

void clear() {
	printf("\e[0;0H\e[2J");
	fflush(stdout);
}

int main(int argc, char ** argv) {
	clear();

	for(;;);
	return 0;
}
