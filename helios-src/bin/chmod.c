#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char ** argv) {
	if(argc > 2) {
		if (chmod(argv[2], strtoul(argv[1], 0, 8)) < 0) {
			fprintf(stderr, "%s: error in chmod(%s, %s)\n", argv[0], argv[2], argv[1]);
			exit(EXIT_FAILURE);
		}
	} else {
		fprintf(stderr, "%s: usage: %s [OPTION] [file]\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}
	return 0;
}
