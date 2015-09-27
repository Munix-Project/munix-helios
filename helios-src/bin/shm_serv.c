/*
 * shm_serv.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#include <shm.h>
#include <stdio.h>

int server_callback(shm_packet_t * packet) {

	return SHM_RET_SUCCESS;
}

int main(int argc, char ** argv) {
	char server_name[16] = "serv_1";
	shm_t * serv = shman_create_server(server_name);
	if(SHM_IS_ERR(serv)) {
		printf("The server '%s' could not be created. Bailing.\n", server_name);
		fflush(stdout);
		return serv;
	}

	printf("Created server with IP '%s'\nListening ...\n", serv->shm_key);
	fflush(stdout);

	int retcode;
	if((retcode = shman_server_listen(serv, server_callback))) {
		printf("Server could not listen (error code: %d). The server has invalid configuration. Bailing.\n", retcode);
		fflush(stdout);
		shman_stop(serv);
		return retcode;
	}
	shman_stop(serv);
	return 0;
}
