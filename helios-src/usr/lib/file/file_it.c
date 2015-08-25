/*
 * file_it.c
 *
 *  Created on: Aug 24, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 4096

/* 'mini library' for file reading */
#define ALL_LINES -1
typedef void (*callback_t)(char*);
void read_all(callback_t call, char * filepath, int nthline) {
	/* General purpose function for reading files line by line (couldn't find 'getline' on libc :S)*/

	FILE * f = fopen(filepath, "r");
	if(f==NULL){
		fprintf(stderr, "%s: no such file or directory\n", filepath);
		exit(EXIT_FAILURE);
	}

	char buff[CHUNK_SIZE] , * line, c = 0;
	memset(buff, 0, CHUNK_SIZE);

	int linecount = 0;
	while(c != EOF){
		int i = 0;
		while((c = getc(f)) != '\n' && c != EOF) buff[i++] = c;

		if(linecount++ == nthline || nthline == ALL_LINES) {
			line = malloc(sizeof(char) * i);
			memcpy(line, buff, i);
			if(call) call(line);
			free(line);

			/*
			 * This if means a certain line was specified, which also means
			 * the programmer didn't want any other lines read, so we're escaping
			 */
			if(nthline != ALL_LINES) return;
		}
	}
}


