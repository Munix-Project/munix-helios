/*
 * stat.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <sys/stat.h>
#include <syscall.h>
#include <stdint.h>
#include <sys/time.h>

#define CMPSTR(str1, str2) !strcmp(str1,str2)

int main(int argc, char ** argv) {
	uint8_t deref = 0;
	if(argc < 2) {
		fprintf(stderr, "%s: expected argument\n", argv[0]);
		return 1;
	}

	char * file = argv[1];

	if(argc > 2) {
		if(CMPSTR(file,"-L"))
			deref = 1;
		file = argv[2];
	}

	struct stat statistics;
	if(deref) {
		if(stat(file, &statistics) < 0) return 1;
	} else {
		if(lstat(file, &statistics) < 0) return 1;
	}

	/* Show file data: */

	printf("> File: '%s'\n", file);
	printf("> Size: %d bytes - Block - size: %d count: %d\n> Type: ",
			statistics.st_size, statistics.st_blksize, statistics.st_blocks);

	if (S_ISDIR(statistics.st_mode))
		printf("directory.\n");
	else if (S_ISFIFO(statistics.st_mode))
		printf("pipe.\n");
	else if (S_ISLNK(statistics.st_mode))
		printf("symlink.\n");
	else if (statistics.st_mode & 0111)
		printf("executable.\n");
	else
		printf("file.\n");

	struct stat * f = &statistics;

	printf("> File properties:\n");
	printf("  - st_mode  0x%x %d\n", (uint32_t)f->st_mode  , sizeof(f->st_mode  ));
	printf("  - st_nlink 0x%x %d\n", (uint32_t)f->st_nlink , sizeof(f->st_nlink ));
	printf("  - st_uid   0x%x %d\n", (uint32_t)f->st_uid   , sizeof(f->st_uid   ));
	printf("  - st_gid   0x%x %d\n", (uint32_t)f->st_gid   , sizeof(f->st_gid   ));
	printf("  - st_rdev  0x%x %d\n", (uint32_t)f->st_rdev  , sizeof(f->st_rdev  ));
	printf("  - st_size  0x%x %d\n", (uint32_t)f->st_size  , sizeof(f->st_size  ));

	printf("0x%x\n", ((uint32_t *)f)[0]);

	return 0;
}
