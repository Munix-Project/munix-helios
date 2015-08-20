/*
 * rm.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char * argv[]) {
	if(argc < 2){
		fprintf(stderr, "%s: argument expected\n", argv[0]);
		return 1;
	}
	unlink(argv[1]);
	return 0;
}
