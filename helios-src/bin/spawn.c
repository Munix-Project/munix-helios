/*
 * spawn.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

int main(int argc, char ** argv) {
	int pid = fork();
	if(!pid) {
		for(;;);
	}
	return 0;
}
