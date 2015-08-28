#include <stdio.h>
#include <unistd.h>
#include <slre.h>
#include <string.h>
#include <stdlib.h>

/************************ REGEX SECTION ************************/

typedef struct {
	char ** matches;
	int count;
} regex_t;

#define SECTOR_COUNT 10
#define MAX_MATCH_COUNT 4096 * SECTOR_COUNT
char * matches[MAX_MATCH_COUNT];

regex_t * rexmatch(char * patt, char * text) {
	int caps_count = 4;
	struct slre_cap caps[caps_count];
	int matchcount = 0, i,j = 0, strl = strlen(text);

	while( j < strl && (i = slre_match(patt, text + j, strl - j, caps, caps_count, 0)) > 0) {
		if(!caps[0].len) continue;

		matches[matchcount] = malloc(sizeof(char*) * caps[0].len);
		memcpy(matches[matchcount], caps[0].ptr, caps[0].len);
		matches[matchcount++][caps[0].len] = '\0';
		j += i;
	}

	if(!matchcount)	return NULL;

	char ** matches_alloc = malloc(sizeof(char**) * matchcount);
	for(int i=0;i<matchcount;i++){
		matches_alloc[i] = malloc(sizeof(char*) * (strlen(matches[i])+1));
		strcpy(matches_alloc[i], matches[i]);
		free(matches[i]);
	}

	regex_t * ret = malloc(sizeof(regex_t));
	ret->count = matchcount;
	ret->matches = matches_alloc;
	return ret;
}

void free_regex(regex_t * reg) {
	for(int i=0;i<reg->count;i++)
		free(reg->matches[i]);
	free(reg->matches);
	free(reg);
}

/************************ GREP SECTION ************************/

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
	regexp 	= malloc(sizeof(char*) * (strlen(argv[1]) + 3));
	sprintf(regexp, "(%s)", argv[1]);

	regex_t * ret = rexmatch(regexp, text);
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
