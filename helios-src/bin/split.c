/*
 * split.c
 *
 *  Created on: Aug 29, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <libstr.h>
#include <slre.h>

int main(int argc, char ** argv) {
	for(int i=0;i<10000000;i++){
		regex_t s = rexmatch("a", "aa");
		free_regex(s);
	}

	return 0;
}
