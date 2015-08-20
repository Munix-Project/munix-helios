/*
 * ex.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, int * argv[]) {
	printf("\e[32mHello friend!\e[0m\n"); fflush(stdout);
	return 0;
}

