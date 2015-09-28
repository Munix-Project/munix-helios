/*
 * shm_client.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#include <shm.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	char * msg = malloc(16);
	strcpy(msg,"hi!");
	char * server_name = malloc(16);
	strcpy(server_name,"serv_1");
	char * client_name = malloc(16);
	strcpy(client_name,"client_1");

	shm_t * client = shman_create_client(server_name, client_name);
	shm_packet_t * p = create_packet(client, server_name, SHM_DEV_SERVER, msg, strlen(msg));
	shman_send_to_network(client, p);
	shman_stop(client);

	return 0;
}
