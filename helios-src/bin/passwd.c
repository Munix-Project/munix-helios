/*
 * passwd.c
 *
 *  Created on: Aug 26, 2015
 *      Author: miguel
 */
#include <unistd.h>

void usage() {
	printf("passwd: usage: passwd username [options]\n"
			"Options:\n"
			"[-d] - remove password from account\n"
			"[-i] - set account inactive\n"
			"[-a] - reactivate an account\n"
			"[-h] - shows this message");
}

int main(int argc, char ** argv) {
	if(argc > 1) {
		int c;
		while((c = getopt(argc, argv, "diah")) != -1) {
			switch(c) {
			case 'd': break;
			case 'i': break;
			case 'a': break;
			case 'h': usage(); return 1; break;
			}
		}


		/* Parse user's password and fill a buffer with master.passwd */

		/* Ask for password */

		/* Compare hashes */

		/* If it's the same: */
			/* Rewrite everything but with the password replaced */
			/* Exit */
			return 0;
		/* Else loop until user gets the password right or presses CTRL+C */
	}else {
		usage();
		return 1;
	}
}
