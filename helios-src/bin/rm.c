/*
 * rm.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* TODO: Improve this tool */

int main(int argc, char * argv[]) {
	if(argc < 2){
		fprintf(stderr, "%s: argument expected\n", argv[0]);
		return 1;
	}

	for(int i = 1; i<argc;i++ )
	unlink(argv[i]);
	return 0;
}
