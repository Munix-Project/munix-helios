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

void print_match(char * filepath, regex_t match) {
	if(just_the_matches) {
		/* Print the matches only */
		if(filepath)
			foreach(mat, match.matches)
				printf("%s%s%s: %s\n", YELLOW_FORE, filepath, NORMAL_TEXT, (char*)mat->value);
		else
			foreach(mat, match.matches)
				printf("%s\n", (char*)mat->value);
	} else  {
		/* Include the whole word, not just the match */
		foreach(word, match.matches_whole){
			/* Grab pointer to the match and its length, and highlight the match on the whole word */
			char * old = strdup(word->value);
			foreach(mat, match.matches) {
				char * repl_color = malloc(sizeof(char *) * (strlen(mat->value) + 14));
				sprintf(repl_color, "\e[32;1m%s\e[0m", (char*)mat->value);

				char * new = (char*)str_replace(old, (char*)(mat->value), repl_color);

				free(repl_color);
				free(old);
				old = new;
			}
			if(filepath)
				printf("%s%s%s: %s\n", YELLOW_FORE, filepath, NORMAL_TEXT, old);
			else
				printf("%s\n", old);
		}
	}
	fflush(stdout);
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

int get_file_size(char * filename) {
	struct stat st;
	if(!stat(filename, &st))
		return st.st_size;
	else
		return -1;
}

char * catfile(char * filepath) {
	size_t ret;
	FILE * f = fopen(filepath, "r");
	int filesize = get_file_size(filepath);
	char * buffer = malloc(sizeof(char*) * filesize);
	if(!buffer) {
		fclose(f);
		return NULL; /*file has size 0. this means the file is no good...*/
	}

	ret = fread(buffer, 1, filesize, f);
	if(ret != filesize) {
		fprintf(stderr, "catfile: '%s' (size: %d): Read error\n", filepath, filesize);
		exit(EXIT_FAILURE);
	}
	fclose(f);
	return buffer;
}

void recursive_callback(fit_dir_cback_dat dat){
	/* Avoid greping processes' folder, the files there can't be read, the program crashes because of it */
	if(str_contains(dat.fullpath, "/proc")) return;
	if(dat.type == FIT_TYPE_FILE) {
		char * contents = catfile(dat.fullpath);
		if(contents){
			regex_t reg = rexmatch(regexp, contents);
			if(reg.matchcount > 0) {
				print_match(dat.fullpath, reg);
				free_regex(reg);
			}
			free(contents);
		}
	}
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
		print_match(NULL, rexmatch(regexp, text));
	}

	if(file_to_grep) {
		/* Grab contents from file and "grep" it */

		/* Check if file is a dir */
		int ret;
		if((ret = validate_file(file_to_grep)) > 0){
			/* It's a valid file */
			print_match(file_to_grep, rexmatch(regexp, catfile(file_to_grep)));
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
