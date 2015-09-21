/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2014 Kevin Lange
 *
 * Authentication methods
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <security/crypt/sha2.h>

int helios_auth(char * user, char * pass) {
	/* Generate SHA512 */
	char * hash = malloc(sizeof(char) * SHA512_DIGEST_STRING_LENGTH);
	SHA512_Data(pass, strlen(pass), hash);

	/* Open up /etc/master.passwd */
	FILE * master = fopen("/etc/master.passwd", "r");
	struct passwd * p;

	/* Read all users and passwords */
	while ((p = fgetpwent(master))) {
		/* Compare passed in pass' hash with master.passwd's pass hash (also the usernames) */
		if (!strcmp(p->pw_name, user) && !strcmp(p->pw_passwd, hash)) {
			/* Success */
			fclose(master);
			free(hash);
			return p->pw_uid;
		}
	}

	free(hash);
	fclose(master);
	return -1;

}

void helios_auth_set_vars(void) {
	int uid = getuid();

	struct passwd * p = getpwuid(uid);

	/* Set USER, HOME and SHELL variables for all users (mainly root) */
	if (!p) {
		char * tmp = malloc(sizeof(char) * 10);
		sprintf(tmp, "%d", uid);
		setenv("USER", strdup(tmp), 1);
		free(tmp);

		setenv("HOME", "/", 1);
		setenv("SHELL", "/bin/sh", 1);
		setenv("WM_THEME", strdup(p->pw_comment), 1);
	} else {
		setenv("USER", strdup(p->pw_name), 1);
		setenv("HOME", strdup(p->pw_dir), 1);
		setenv("SHELL", strdup(p->pw_shell), 1);
		setenv("WM_THEME", strdup(p->pw_comment), 1);
	}
	endpwent();

	setenv("PATH", "/usr/bin:/bin", 0);
	chdir(getenv("HOME"));
}
