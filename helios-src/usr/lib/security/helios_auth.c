/*
 * heliosos_auth.c
 *
 *  Created on: Aug 19, 2015
 *      Author: miguel
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <security/crypt/sha2.h>

/* God damn compiler didn't want to compile, so I put this here and now it does -.-' */
void __stack_chk_fail(void) {}

char hash [SHA512_DIGEST_STRING_LENGTH];

/* FIXME: */
int helios_auth(char * user, char * pass) {
	/* XXX BYPASS AUTHORIZATION FOR NOW, SHA2 SEEMS TO BE NOT WORKING */
	if(!strcmp(user,"root") && !strcmp(pass,"toor")) return 1;
	else return -1;

	/* Generate SHA512 */
	/*char * str = SHA512_Data(pass, strlen(pass), hash);
	fprintf("%d",str); fflush(stdout);
	//fprintf("strlen: %d str: %s", strlen(str), str);
	 */

	/* Open up /etc/master.passwd */
	FILE * master = fopen("/etc/master.passwd", "r");
	struct passwd * p;

	/* Read all users and passwords */
	while ((p = fgetpwent(master))) {
		/* Compare passed in pass' hash with master.passwd's pass hash (also the usernames) */
		if (!strcmp(p->pw_name, user) && !strcmp(p->pw_passwd, hash)) {
			/* Success */
			fclose(master);
			return p->pw_uid;
		}
	}

	fclose(master);
	return -1;

}

void helios_auth_set_vars(void) {
	int uid = getuid();

	struct passwd * p = getpwuid(uid);

	/* Set USER, HOME and SHELL variables for all users (mainly root) */
	/*FIXME*/
	setenv("USER", "root", 1);
	setenv("HOME", "/", 1);
	setenv("SHELL", "/bin/sh", 1);

	/*if (!p) {
		char tmp[10];
		sprintf(tmp, "%d", uid);
		setenv("USER", strdup(tmp), 1);
		setenv("HOME", "/", 1);
		setenv("SHELL", "/bin/sh", 1);
	} else {
		setenv("USER", strdup(p->pw_name), 1);
		setenv("HOME", strdup(p->pw_dir), 1);
		setenv("SHELL", strdup(p->pw_shell), 1);
		setenv("WM_THEME", strdup(p->pw_comment), 1);
	}*/
	endpwent();

	setenv("PATH", "/usr/bin:/bin", 0);
	chdir(getenv("HOME"));
}
