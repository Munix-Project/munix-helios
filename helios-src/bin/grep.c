#include <stdio.h>
#include <unistd.h>
#include <slre.h>
#include <string.h>

char * regexp;
char * text;

void usage() {
	printf("Usage: grep [OPTION] PATTERN TEXT\n");
	fflush(stdout);
}

int main(int argc, char ** argv) {
	if(argc < 2) {
		usage();
		return 1;
	}

	regexp 	= argv[1];
	text 	= argv[2];

	int caps_count = 4;
	struct slre_cap caps[caps_count];
	if(slre_match(regexp, text, strlen(text), caps, caps_count, 0) > 0) {
		printf("Match: '%s'\n", caps[0].ptr);
	} else {
		printf("Error parsing '%s'\n", text);
	}

	fflush(stdout);

	return 0;
}
