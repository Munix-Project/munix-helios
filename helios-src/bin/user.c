/*
 * user.c
 *
 *  Created on: Aug 22, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint8_t fl_usradd = 0;
uint8_t fl_usrdel = 0;
uint8_t fl_usradd_to_group = 0;
uint8_t fl_usrdel_from_group = 0;
uint8_t groupadd = 0;
uint8_t groupdel = 0;

/* 'mini library' for file reading */
#define CHUNK_SIZE 4096
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

void read_singleline(char * line) {
	if(!line) printf("|\n");
	else printf("%s|\n", line);
	fflush(stdout);
}

int main(int argc, char ** argv) {
	if(argc<2)
		exit(EXIT_FAILURE);

	read_all(read_singleline, argv[1], ALL_LINES);
	return 0;
}
