/*
 * clear.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>

int main(int argc, char ** argv) {
	printf("\e[0;0H\e[2J");
	fflush(stdout);
	return 0;
}

