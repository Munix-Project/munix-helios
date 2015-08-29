/*
 * file_it.c
 *
 *  Created on: Aug 24, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <file_it.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void __stack_chk_fail(void) {}

#define CHUNK_SIZE 4096

/********************** File Reader **********************/

void file_read_all(file_callback_t call, char * filepath, int nthline) {
	/* General purpose function for reading files line by line (couldn't find 'getline' on libc :S)*/

	FILE * f = fopen(filepath, "r");
	if(f==NULL){
		fprintf(stderr, "%s: no such file or directory\n", filepath);
		exit(EXIT_FAILURE);
	}

	char buff[CHUNK_SIZE] , * line, c = 0;
	memset(buff, 0, CHUNK_SIZE);

	int linecount = 0;
	while(c != EOF){
		int i = 0;
		while((c = getc(f)) != '\n' && c != EOF) buff[i++] = c;

		if(linecount++ == nthline || nthline == ALL_LINES) {
			line = malloc(sizeof(char) * i);
			memcpy(line, buff, i);
			if(call) call(line);
			free(line);

			/*
			 * This if means a certain line was specified, which also means
			 * the programmer didn't want any other lines read, so we're escaping
			 */
			if(nthline != ALL_LINES) return;
		}
	}
}

/********************** Directory crawler ***********************/

#define IS_PATH_BAD(path) !strcmp(path, ".") || !strcmp(path, "..") || !strcmp(path, "lost+found")

void dir_crawl(fit_dir_cback_t call, char * path, int depth, int maxdepth) {
	if(maxdepth>=0 && depth > maxdepth) return;

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
			fit_dir_cback_dat pack = { .fullpath = fullpath_, .type = FIT_TYPE_DIR, .name=nodename, .size = statbuff.st_size};
			if(call) call(pack);
			dir_crawl(call, fullpath, depth + 1, maxdepth);
		}else { /* Handle links: */
			uint8_t was_link = 0;
			if(S_ISLNK(statbuff.st_mode)) {
				stat(fullpath, &statbuff);
				char * link = malloc(CHUNK_SIZE);
				readlink(fullpath_, link, CHUNK_SIZE);
				sprintf(fullpath_, "%s/%s", path, link);
				free(link);
				was_link = 1;
			}

			fit_dir_cback_dat pack = { .fullpath = fullpath_, .name=nodename, .size = statbuff.st_size};
			if(was_link)
				pack.type = FIT_TYPE_LINK; /* Link */
			else {
				if(S_ISBLK(statbuff.st_mode))
					pack.type = FIT_TYPE_BLOCK; /* Block device */
				else if(statbuff.st_mode & 0111)
					pack.type = FIT_TYPE_EXEC; /* Executable */
				else
					pack.type = FIT_TYPE_FILE; /* Regular file */
			}

			/* Else it's a regular file/not a directory */
			if(call) call(pack);
		}
		dire = readdir(dir);
	}
	closedir(dir);
}
