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

	text 	= argv[2];
	regexp 	= argv[1];

	regex_t * ret = (regex_t*)rexmatch(regexp, text);
	if(!ret){
		printf("No matches found\n");
	} else {
		for(int i=0;i<ret->count;i++)
			printf("%s\n", ret->matches[i]);

		free_regex(ret);
	}

	free(regexp);
	fflush(stdout);
	return 0;
}
