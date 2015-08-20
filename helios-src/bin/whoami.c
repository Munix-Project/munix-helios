/*
 * whoami.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>

int main(int argc, char ** argv) {
	struct passwd * p = getpwuid(getuid());
	if (!p) return 0;
	fprintf(stdout, "%s\n", p->pw_name);
	endpwent();
	return 0;
}



