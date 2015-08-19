/*
 * uname.c
 *
 *  Created on: Aug 18, 2015
 *      Author: miguel
 *
 *  uname: Prints the kernel info
 */

#include <sys/utsname.h>
#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>
#include <unistd.h>

#define FLAG_SYSNAME  0x01
#define FLAG_NODENAME 0x02
#define FLAG_RELEASE  0x04
#define FLAG_VERSION  0x08
#define FLAG_MACHINE  0x10

#define FLAG_ALL (FLAG_SYSNAME|FLAG_NODENAME|FLAG_RELEASE|FLAG_VERSION|FLAG_MACHINE)

#define _ITALIC "\033[3m"
#define _END    "\033[0m\n"

int main(int argc, char * argv[]) {
	struct utsname uts;

	int c,
		flags = 0,
		space = 0;

	while((c = getopt(argc, argv, "ahmnrsv")) != -1) {
		switch(c) {
		case 'a': flags |= FLAG_ALL;  break;
		case 's': flags |= FLAG_SYSNAME; break;
		case 'n': flags |= FLAG_NODENAME; break;
		case 'r': flags |= FLAG_RELEASE; break;
		case 'v': flags |= FLAG_VERSION; break;
		case 'm': flags |= FLAG_MACHINE; break;
		case 'h':
		default: /*TODO*/ break;
		}
	}
	if(!flags)
		flags = FLAG_SYSNAME;

	uname(&uts);

	printf("\e[41;1;33m");

	if (flags & FLAG_SYSNAME) {
		if (space++) printf(" ");
		printf("%s", uts.sysname);
	}

	if (flags & FLAG_NODENAME) {
		if (space++) printf(" ");
		printf("%s", uts.nodename);
	}

	if (flags & FLAG_RELEASE) {
		if (space++) printf(" ");
		printf("%s", uts.release);
	}

	if (flags & FLAG_VERSION) {
		if (space++) printf(" ");
		printf("%s", uts.version);
	}

	if (flags & FLAG_MACHINE) {
		if (space++) printf(" ");
		printf("%s", uts.machine);
	}

	printf("\e[0m\n");

	return 0;
}

