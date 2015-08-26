/*
 * grep.c
 *
 *  Created on: Aug 26, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <regex.h>
#include <unistd.h>

#define CHUNK_SIZE 4096

void usage() {
	printf("Usage: grep [OPTION] PATTERN TEXT\n");
	fflush(stdout);
}

static int compile_regex(regex_t * reg, const char * patt, int flags) {

	return 1;
}

static int match_regex(regex_t * reg, const char * text) {

	return 1;
}

int main(int argc, char ** argv) {
	if(argc < 2) {
		usage();
		return 1;
	}

	char c;
	char * patt, * text;
	while((c = getopt(argc, argv, "he:")) != -1) {
		switch(c) {
		case 'h': usage(); break;
		}
	}

	patt = argv[argc-2];
	text = argv[argc-1];

	regex_t r;
	compile_regex(&r, patt, REG_EXTENDED|REG_NEWLINE);
	match_regex(&r, text);

	return 0;
}
