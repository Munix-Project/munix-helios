
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* TODO: Make a proper move without having to remove the original file */

int main(int argc, char ** argv) {
	if(argc < 3) {
		fprintf(stderr, "%s: usage: [source] [destination]\n", argv[0]);
		return 1;
	}

	if(!strcmp(argv[1],argv[2])) return 0; /* Source has same name and same destination, no point in doing a move */

	/* First make a copy: */
	char buff[strlen(argv[0]) + strlen(argv[1]) + strlen(argv[2]) + 3];
	sprintf(buff, "cp %s %s", argv[1], argv[2]);
	system(buff);

	/* Then remove the original: */
	char buff2[strlen(argv[1]) + 3];
	sprintf(buff2, "rm %s", argv[1]);
	system(buff2);
	return 0;
}
