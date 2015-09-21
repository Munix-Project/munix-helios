#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <file_it.h>

/* flags */
uint8_t byname = 0;
uint8_t bytype = 0;
uint8_t bysize = 0;
uint8_t byexec = 0;
uint8_t limit_depth = 0;
uint8_t delete_files = 0;

char * byname_str;
char * bytype_str;
char * toppath = ".";
int bysize_int = 0;
int maxdepth = 0;

static int files_found = 0;
static int matches_found = 0;

void update_stats(fit_dir_cback_dat dat) {
	matches_found++;
	if(dat.type != FIT_TYPE_DIR) files_found++;
}

void find(fit_dir_cback_dat dat){
	/* Do the filtering here using the flags */
	int size = dat.size;
	char * fullpath = dat.fullpath;
	char * name = dat.name;
	uint8_t type = dat.type;

	/* Decide to whether show the data or not: */
	if(!byexec && !byname && !bysize && !bytype){ /* All conditions are off, show all matches */
		goto match_found;
	} else {
		/* TODO: Add regex to the name matching */

		/* Filter it */
		int trycount = 1;
		if(bytype) trycount = strlen(bytype_str);

		for(int i=0; i< trycount; i++) /* Try for all options given in type */
		{
			uint8_t match_types = ((byexec && type == FIT_TYPE_EXEC) || (bysize && bysize_int>=size) || (bytype && bytype_str[i]==type));
			uint8_t match_name = (byname && !strcmp(byname_str, name));

			/* Cover all possibilities: */
			if(match_types && !byname)
				goto match_found;
			if(match_name && !bytype)
				goto match_found;
			if(match_name && match_types)
				goto match_found;
		}
	}

	/* No match found */
	return;
	match_found:
		/* Match found */
		printf("%s\n", fullpath);
		update_stats(dat);
		if(delete_files) {
			/* TODO, don't want to be deleting things just yet */
		}
		fflush(stdout);
}

void usage(char * argv) {
	printf("%s: usage: \n[-t] - type.\nAvailable: \n\te - executable"
			"\n\tf - file"
			"\n\td - directory"
			"\n\tb - block"
			"\n\tl - link"
			"[-n] - name\n"
			"[-s] - size (matches bigger or equal than this size)\n"
			"[-e] - matches executables\n"
			"[-m] - max depth\n"
			"[-d] - deletes the file found\n", argv);

	fflush(stdout);
}

int main(int argc, char ** argv) {
	if(argc > 1) {
		if(argv[1][0] != '-')
			toppath = argv[1];

		int c;
		while((c = getopt(argc, argv, "n:es:t:m:d?")) != -1) {
			switch(c) {
			case 'n':
				byname = 1;
				byname_str = optarg;
				break;
			case 'e': byexec = 1; break;
			case 's':
				bysize = 1;
				bysize_int = atoi(optarg);
				break;
			case 't':
				bytype = 1;
				bytype_str = optarg;
				break;
			case 'm':
				limit_depth = 1;
				maxdepth = atoi(optarg);
				break;
			case 'd': delete_files = 1; break;
			case '?': usage(argv[0]); exit(EXIT_SUCCESS); break;
			}
		}
		if(!strcmp(toppath,".")) {
			/* Always . on the current dir */
			fit_dir_cback_dat dat = { .fullpath = ".", .type = FIT_TYPE_DIR, .name="." };
			find(dat);
		}
	} else {
		/* Always . on the current dir */
		fit_dir_cback_dat dat = { .fullpath = ".", .type = FIT_TYPE_DIR, .name="." };
		find(dat);
	}
	if(!strcmp(toppath,"~"))
		toppath = getenv("HOME");

	dir_crawl(find, toppath, 0, limit_depth ? maxdepth : IGNORE_DEPTH);
	printf("\n%d matches / %d files  / %d folders found\n", matches_found, files_found, matches_found - files_found);
	return 0;
}
