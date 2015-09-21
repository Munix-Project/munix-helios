/* This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2014 Kevin Lange
 */
/*
 * cat
 *
 * Concatenates files together to standard output.
 * In a supporting terminal, you can then pipe
 * standard out to another file or other useful
 * things like that.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define CHUNK_SIZE 4096

void cat(int filedir) {
	while (1) {
		char buf[CHUNK_SIZE];
		memset(buf, 0, CHUNK_SIZE);
		ssize_t r = read(filedir, buf, CHUNK_SIZE);
		if (!r) return;
		write(STDOUT_FILENO, buf, r);
	}
}

int main(int argc, char**argv) {
	int ret = 0;
	if(argc == 1) cat(0);

	for (int i = 1; i < argc; ++i) {
		int filedir = open(argv[i], O_RDONLY);
		if (filedir == -1) {
			fprintf(stderr, "%s: %s: no such file or directory\n", argv[0], argv[i]);
			ret = 1;
			continue;
		}

		struct stat _stat;
		fstat(filedir, &_stat);
		if (S_ISDIR(_stat.st_mode)) {
			fprintf(stderr, "%s: %s: Is a directory\n", argv[0], argv[i]);
			close(filedir);
			ret = 1;
			continue;
		}

		cat(filedir);
		close(filedir);
	}
	return ret;
}
