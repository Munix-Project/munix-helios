/*
 * shm.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 *
 *  This API allows communication with the shared memory manager. It sends and receives commands to and from it.
 */

#include <shm.h>
#include <syscall.h>
#include <stdlib.h>

/* Size of channel used between any process and the shared memory manager */
size_t shman_size = SHMAN_SIZE;

static char * shm_manager;
static uint8_t is_shman_init = 0;

static void * get_cmd_channel() {
	return (void*)syscall_shm_obtain(SHM_RESKEY_SHMAN_CMD, &shman_size);
}

/*
 * shman_init - Initializes the shared memory channel used to communicate with the shared memory manager
 */
char * shman_init() {
	shm_manager = get_cmd_channel();
	is_shman_init = 1;
	return shm_manager;
}

void shman_stop(shm_t * shm_dev) {
	if(is_shman_init) {
		shman_send(shm_dev->dev_type + 3, shm_dev, sizeof(shm_t));
		free(shm_dev);
	}
}

/*
 * shm_send - Sends a command and packet to the shared memory manager.
 * 	The commands are:
 * 		* 1 - Create a new server
 * 		* 2 - Create a new client
 * 		* 3 - Send packet to server
 * 	[char cmd] - command to send
 * 	[char * packet] - packet/data to send
 */
void * shman_send(char cmd, void * packet, char packet_size) {
	if(!is_shman_init)
		shman_init();

	if(packet) { /* Send packet to the shared memory manager */
		/* Copy whatever the response variable points to to the shared memory with offset of 1 byte */
		shm_manager[1] = packet_size; /* second byte contains the size of the response */
		memcpy(shm_manager + 2, packet, packet_size); /* the third byte contains the start of the response data */
	}
	shm_manager[0] = cmd;
	shm_wait_for_ack(shm_manager);
	if(!shm_manager[1] && shm_manager[2])
		return shm_manager[2]; /* Oops, this result is invalid. Return the error number */
	else
		return &shm_manager[2]; /* grab shm manager's acknowledge result, if there is one */
}

/* Function wrappers: */
shm_t * shman_create_server(char * server_id) {
	shm_t * server_from_shm = (shm_t*)shman_send(SHM_CMD_NEW_SERVER, server_id, strlen(server_id));
	if(server_from_shm > SHM_NOERR) {
		shm_t * new_server = malloc(sizeof(shm_t));
		memcpy(new_server, server_from_shm, sizeof(shm_t));
		return new_server;
	} else {
		return server_from_shm;
	}
}

shm_t * shman_create_client(char * server_id) {
	shm_t * client_from_shm = (shm_t*)shman_send(SHM_CMD_NEW_CLIENT, server_id, strlen(server_id));
	if(client_from_shm > SHM_NOERR) {
		shm_t * new_client = malloc(sizeof(shm_t));
		memcpy(new_client, client_from_shm, sizeof(shm_t));
		return new_client;
	} else {
		return client_from_shm;
	}
}
