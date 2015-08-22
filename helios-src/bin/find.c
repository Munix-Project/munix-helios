/*
 * find.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define IS_PATH_BAD(path) !strcmp(path, ".") || !strcmp(path, "..") || !strcmp(path, "lost+found")

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

enum TYPES {
	TYPE_DIR 	= 'd',
	TYPE_EXEC 	= 'e',
	TYPE_LINK 	= 'l',
	TYPE_FILE 	= 'f',
	TYPE_BLOCK 	= 'b'
};

typedef struct {
	char * fullpath;
	char * name;
	uint8_t type;
	int size;
} callback_dat;

typedef void (*callback_t)(callback_dat);

void crawl(callback_t call, char * path, int depth) {
	if(limit_depth && depth > maxdepth) return;

	/* Remove '/' from the end of the path */
	int pathlen = strlen(path), ptr = pathlen - 1;
	if(path[pathlen - 1] == '/'){
		/* Search for normal char that isn't '/' counting from the back */
		for(int i = pathlen - 1; i>0; i--)
			if(path[i]!='/'){ptr = i; break;} /* Found offset */
		path[ptr+1] = '\0';
	}

	DIR * dir = opendir(path);
	if(dir == NULL){
		fprintf(stderr, "find: %s: No such directory\n", path);
		exit(EXIT_FAILURE);
	}

	struct dirent * dire = readdir(dir);
	while(dire != NULL) {
		char * nodename = dire->d_name;
		if(IS_PATH_BAD(nodename)) {
			dire = readdir(dir);
			continue;
		}

		/* Go recursive here */
		struct stat statbuff;
		char fullpath[strlen(path) + strlen(nodename) + 1];
		sprintf(fullpath, "%s/%s", path, nodename);
		lstat(fullpath, &statbuff);

		/* Fix up fullpath: */
		char * fullpath_ = fullpath;
		int i=0;
		for(i=0; fullpath[i]=='/'; i++,fullpath_++); /* Move offset because of redudant /'s */
		if(i>0) fullpath_--; /* Not so far */

		/* Is it a directory? */
		if(S_ISDIR(statbuff.st_mode)){
			/* Also match this directory to the search */
			callback_dat pack = { .fullpath = fullpath_, .type = TYPE_DIR, .name=nodename, .size = statbuff.st_size};
			if(call) call(pack);
			crawl(call, fullpath, depth + 1);
		}else { /* Handle links: */
			uint8_t was_link = 0;
			if(S_ISLNK(statbuff.st_mode)) {
				stat(fullpath, &statbuff);
				char * link = malloc(4096);
				readlink(fullpath_, link, 4096);
				sprintf(fullpath_, "%s/%s", path, link);
				free(link);
				was_link = 1;
			}

			callback_dat pack = { .fullpath = fullpath_, .name=nodename, .size = statbuff.st_size};
			if(was_link)
				pack.type = TYPE_LINK; /* Link */
			else {
				if(S_ISBLK(statbuff.st_mode))
					pack.type = TYPE_BLOCK; /* Block device */
				else if(statbuff.st_mode & 0111)
					pack.type = TYPE_EXEC; /* Executable */
				else
					pack.type = TYPE_FILE; /* Regular file */
			}

			/* Else it's a regular file/not a directory */
			if(call) call(pack);
		}
		dire = readdir(dir);
	}
	closedir(dir);
}

static int files_found = 0;
static int matches_found = 0;

void update_stats(callback_dat dat) {
	matches_found++;
	if(dat.type != TYPE_DIR) files_found++;
}

void find(callback_dat dat){
	/* Do the filtering here using the flags */
	int size = dat.size;
	char * fullpath = dat.fullpath;
	char * name = dat.name;
	uint8_t type = dat.type;

	/* Decide to whether to show data or not: */
	if(!byexec && !byname && !bysize && !bytype){ /* All conditions are off, show all matches */
		printf("%s\n", fullpath);
		update_stats(dat);
	}
	else {
		/* Filter it */
		/* TODO: Add regex to the name matching */
		int trycount = 1;
		if(bytype) trycount = strlen(bytype_str);

		for(int i=0; i< trycount; i++) /* Try for all options given in type */
		{
			if((byexec && type==TYPE_EXEC) ||
			   (byname && !strcmp(byname_str, name)) ||
			   (bysize && bysize_int>=size) ||
			   (bytype && bytype_str[i]==type))
			{
				printf("%s\n", fullpath);
				update_stats(dat);
				if(delete_files) {
					/* TODO, don't want to be deleting things just yet */
				}
			}
		}
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
			"[-d] - deletes the file found", argv);

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
			case TYPE_EXEC: byexec = 1; break;
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
	} else {
		/* Always . on the current dir */
		callback_dat dat = { .fullpath = ".", .type = TYPE_DIR, .name="." };
		find(dat);
	}
	crawl(find, toppath, 0);
	printf("\n%d matches / %d files  / %d folders found\n", matches_found, files_found, matches_found - files_found);
	return 0;
}
