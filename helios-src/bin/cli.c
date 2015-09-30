/*
 * shm_client.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#include <shm.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	char msg[16] = "hi!";
	shm_t  * client = shman_create_client("serv_1", "client_1");
	shm_packet_t * server_packet = client_send(client, msg, strlen(msg));

	printf("Server's Data: %s\n", server_packet->dat);
	fflush(stdout);

	shman_stop(client);
	return 0;
}
