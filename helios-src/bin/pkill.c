/*
 * pkill.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char ** argv) {
	if(argc < 2) {
		fprintf(stderr, "%s: usage: [process-name]\n", argv[0]);
		return 1;
	}

	/* Open the directory */
	DIR * dirp = opendir("/proc");

	struct dirent * ent = readdir(dirp);
	while (ent != NULL) {
		if (ent->d_name[0] >= '0' && ent->d_name[0] <= '9') {
			char * pid_no = ent->d_name;

			char tmp[100], line[50];
			sprintf(tmp, "/proc/%s/status", pid_no);
			FILE * f = fopen(tmp, "r");
			fgets(line, 256, f);

			char * pid_name[40];
			memset(pid_name,0,40);
			memcpy(pid_name, line + 6, strlen(line+6) - 1);

			if(!strcmp(pid_name, argv[1])) {
				/* Process found! kill it before it lays eggs! */
				if(!fork()) {
					char * tok[] = {"kill", pid_no, NULL};
					execvp(tok[0], tok);
				}
			}

			fclose(f);
		}

		ent = readdir(dirp);
	}
	closedir(dirp);

	return 0;
}
