/*
 * rename.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	/* Same as mv but moves to the same folder without the user's specification */
	if(argc < 3) {
		fprintf(stderr, "%s: usage: %s [file] [newname]\n", argv[0], argv[0]);
		return 1;
	}

	/* Grab diffname from argv[1]. Replace only the name, keep the path, even if it doesn't exist on the string */

	/* First get the size of the path, if there is any: */
	int path_size = strrchr(argv[1], '/') - argv[1] + 1;
	if(path_size < 1) path_size = 0;

	/* Now set diffname with the path and the filename: */
	int malloc_size = sizeof(char) * (path_size + strlen(argv[2]) + 1);
	char * diffname = malloc(malloc_size);
	memset(diffname, 0, malloc_size);

	if(path_size > 0)
		memcpy(diffname, argv[1], path_size);
	strcat(diffname, argv[2]); strcat(diffname, "\0");

	/* Great, it's done. Build the command now: */
	int buff_size = 5 + strlen(argv[1]) + path_size + strlen(argv[2]);
	char buff[buff_size];
	memset(buff, 0, buff_size);
	sprintf(buff, "mv %s %s", argv[1], diffname);
	free(diffname);

	/* And run it! */
	system(buff);
	return 0;
}
