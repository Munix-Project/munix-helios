/*
 * cp.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

/* TODO: Improve this tool by allowing more options to make it recursive and copy multiple files (with wildcard or not) */
/* TODO: Allow copy of directories, not just files */

#define CHUNK_SIZE 4096

#define IS_IT_A_DIR(path) struct stat statbuff; \
		stat(path, &statbuff); \
		if(S_ISDIR(statbuff.st_mode))

/* Copies a single file: */
static void copy(FILE * fin, FILE * fout) {
	int numBytes;
	char buf[50];
	while((numBytes=fread(buf, 1, 50, fin)))
		fwrite(buf, 1, numBytes, fout);
}

int main(int argc, char ** argv) {
	FILE * fin, * fout;

	if(argc < 3) {
		fprintf(stderr, "usage: %s [source] [destination]\n", argv[0]);
		return 1;
	}

	if(!strcmp(argv[1], argv[2])) return 0; /* Same destination, no point in copying */

	if(!strcmp(argv[1],"~")) strcpy(argv[1], getenv("HOME"));
	if(!strcmp(argv[2],"~")) strcpy(argv[2], getenv("HOME"));

	/* Set fin and fout: */

	fin = fopen(argv[1], "r");
	if(!fin) {
		fprintf(stderr, "%s: %s: no such file or directory\n", argv[0], argv[1]);
		return 2;
	}

	/* Grab destination path data: */
	IS_IT_A_DIR(argv[2]) {
		char * src_filename = strrchr(argv[1], '/');
		if(!src_filename)
			src_filename = argv[1];

		char *target_path = malloc((strlen(argv[2]) + strlen(src_filename) + 2) * sizeof(char));
		sprintf(target_path, "%s/%s", argv[2], src_filename);
		/* File target determined: */
		fout = fopen(target_path, "w");
		free(target_path);
	} else {
		fout = fopen(argv[2], "w");
	}

	/* Now copy the file! */
	copy(fin, fout);

	fclose(fin);
	fclose(fout);

	return 0;
}
