/*
 * passwd.c
 *
 *  Created on: Aug 26, 2015
 *      Author: miguel
 */

void usage() {
	/* TODO */
}

int main(int argc, char ** argv) {
	if(argc > 1) {
		/* TODO */

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
