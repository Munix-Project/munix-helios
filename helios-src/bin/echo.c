/*
 * echo.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>

int main(int argc, char ** argv){
	if(argc > 1)
		for(int i=1;i<argc;i++)
			printf("%s ", argv[i]);
	printf("\n");
	fflush(stdout);
	return 0;
}
