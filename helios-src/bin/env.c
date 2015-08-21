/*
 * env.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <stdio.h>

int main(int argc, char ** argv) {
	unsigned int nulls = 0;
	for (unsigned int i = 0;; i++) {
		if (!argv[i]) {
			if (++nulls == 2)
				break;
			else continue;
		}
		if (nulls == 1)
			printf("%s\n", argv[i]);
	}
}
