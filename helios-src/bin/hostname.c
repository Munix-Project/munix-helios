/*
 * hostname.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <syscall.h>

#define ROOT_UID 0

int main(int argc, char * argv[]) {
	if ((argc > 1 && argv[1][0] == '-') || (argc < 2)) {
		char tmp[256];
		syscall_gethostname(tmp);
		printf("%s\n", tmp);
		return 0;
	} else {
		if (syscall_getuid() != ROOT_UID) {
			fprintf(stderr,"Must be root to set hostname.\n");
			return 1;
		} else {
			syscall_sethostname(argv[1]);
			FILE * file = fopen("/etc/hostname", "w");
			if (!file) {
				return 1;
			} else {
				fprintf(file, "%s\n", argv[1]);
				fclose(file);
				return 0;
			}
		}
	}
	return 0;
}
