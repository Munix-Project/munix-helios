/*
 * shm_serv.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#include <shm.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	shm_t * serv = shman_create_server("serv_1");

	printf("ret: s1 - %s\n", serv->shm_key);
	fflush(stdout);

	shman_stop(serv);
	return 0;
}
