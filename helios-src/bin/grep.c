#include <stdio.h>
#include <unistd.h>
#include <slre.h>
#include <file_it.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libstr.h>

#define YELLOW_FORE "\e[33;1m"
#define NORMAL_TEXT "\e[0m"

char * regexp;
char * text;
char * path_to_search = NULL;
char * file_to_grep = NULL;
uint8_t just_the_matches = 0;

enum NODE_TYPE{
	NODE_FILE,
	NODE_DIR,
	NODE_NOEXIST
};

void usage() {
	printf("grep: usage: grep PATTERN [OPTION]\n"
			"Options:\n"
			"[-f] - Apply regex to file's contents;\n"
			"[-r] - Apply regex to all files on a directory recursively\n"
			"[-h] - Shows this message\n\n"
			"If no options are given, then the second parameter MUST be plain text which the pattern will match, unless pipes are being used.");
	fflush(stdout);
}

void err(char * msg) {
	fprintf(stderr, "grep: %s\n", msg);
	fflush(stdout);
	exit(EXIT_FAILURE);
}

void print_match(char * filepath, regex_t * match) {
	if(match){
		if(just_the_matches) {
			/* Print the matches only */
			if(filepath)
				for(int i=0;i<match->matchcount;i++)
					printf("%s%s%s: %s\n", YELLOW_FORE, filepath, NORMAL_TEXT, match->matches[i]);
			else
				for(int i=0;i<match->matchcount;i++)
					printf("%s\n", match->matches[i]);
		} else  {
			/* Include the whole word, not just the match */
			for(int i=0;i<match->wordcount;i++) {
				/* Grab pointer to the match and its length, and highlight the match on the whole word */
				char * updated = match->matches_whole[i];

				for(int j=0;j<match->matchcount;j++) {
					char * repl_color = malloc(sizeof(char *) * (strlen(updated) + 14));
					sprintf(repl_color, "\e[32;1m%s\e[0m", match->matches[i]);

					char * tmp = (char*)str_replace(updated, match->matches[i], repl_color);
					updated = tmp;

					free(repl_color);
				}
				if(filepath)
					printf("%s%s%s: %s\n", YELLOW_FORE, filepath, NORMAL_TEXT, updated);
				else
					printf("%s\n", updated);
				free(updated);
			}
		}
		free_regex(match);
		fflush(stdout);
	}
}

uint8_t get_node_type(char * nodepath) {
	int type = NODE_NOEXIST;

	int node = open(nodepath, O_RDONLY);
	if (node == -1) return type;

	struct stat _stat;
	fstat(node, &_stat);
	if (S_ISDIR(_stat.st_mode)) type = NODE_DIR;
	else type = NODE_FILE;

	close(node);

	return type;
}

int8_t validate_file(char * filepath) {
	switch(get_node_type(filepath)) {
	case NODE_FILE: return 1; break;
	case NODE_DIR: return 0; break;
	case NODE_NOEXIST: return -1; break;
	}
	return -2; /* wut */
}

int8_t validate_dir(char * dirpath) {
	switch(get_node_type(dirpath)) {
	case NODE_DIR: return 1; break;
	case NODE_FILE: return 0; break;
	case NODE_NOEXIST: return -1; break;
	}
	return -2; /* wut */
}

int get_file_size(FILE * f) {
	int size = -1;
	fseek(f, 0L, SEEK_END);
	size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	return size;
}

char * catfile(char * filepath) {
	FILE * f = fopen(filepath, "r");
	int filesize = get_file_size(f);
	char * buffer = malloc(sizeof(char*) * (filesize + 1));
	fread(buffer, filesize, 1, f);
	fclose(f);
	buffer[filesize] = '\0';
	return buffer;
}

void recursive_callback(fit_dir_cback_dat dat){
	char * contents = catfile(dat.fullpath);
	print_match(dat.fullpath, (regex_t*)rexmatch(regexp, contents));
	free(contents);
}

int main(int argc, char ** argv) {
	if(argc < 2) {
		usage();
		return 1;
	}

	/* the first argument is always a pattern */
	regexp = argv[1];
	text = argv[2];

	int c;
	while((c = getopt(argc, argv, "hmf:r:")) != -1) {
		switch(c) {
		case 'f': file_to_grep = optarg; break;
		case 'r': path_to_search = optarg; break;
		case 'm': just_the_matches = 1; break;
		case 'h': usage(); return 0; break;
		}
	}

	if(!file_to_grep && !path_to_search) {
		/* Matching a simple input with no recursive search nor file reading */
		/* TODO: In the future Read from the shell pipe */
		print_match(NULL, (regex_t*)rexmatch(regexp, text));
	}

	if(file_to_grep) {
		/* Grab contents from file and "grep" it */

		/* Check if file is a dir */
		int ret;
		if((ret = validate_file(file_to_grep)) > 0){
			/* It's a valid file */
			print_match(file_to_grep, (regex_t*)rexmatch(regexp, catfile(file_to_grep)));
		} else {
			char msg[100];
			if(!ret)
				sprintf(msg, "'%s' is a directory", file_to_grep);
			else
				sprintf(msg, "'%s' no such file", file_to_grep);
			err(msg);
		}
	}

	if(path_to_search) {
		/* Crawl through the path and grep all files! */

		/* Check if dir is a file */
		int ret;
		if((ret = validate_dir(path_to_search)) > 0) {
			/* It's a valid directory */
			dir_crawl(recursive_callback, path_to_search, 0, IGNORE_DEPTH);
		} else {
			char msg[100];
			if(!ret)
				sprintf(msg, "'%s' is a file", path_to_search);
			else
				sprintf(msg, "'%s' no such directory", path_to_search);
			err(msg);
		}

	}
	return 0;
}
