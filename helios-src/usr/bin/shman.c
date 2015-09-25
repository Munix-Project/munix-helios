/*
 * shmman.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 *
 *  Initializes and manages the shared memory
 */
#include <shm.h>
#include <stdio.h>
#include <hashmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include <spinlock.h>

static hashmap_t * shm_network;
static int servers_online = 0;
static char * shm_cmd;
static size_t shm_cmd_size = 2;

static void shm_network_init() {
	shm_network = hashmap_create(1);
	shm_cmd = (char*) syscall_shm_obtain(SHM_RESKEY_SHMAN_CMD, &shm_cmd_size);
}

static void thread_sleep() {
	usleep(POLL_THREAD_SLEEP_USEC);
}

static void add_server(shm_t * server_dev) {
	char hashkey[16];
	sprintf(hashkey, "%d", servers_online);
	hashmap_set(shm_network, hashkey, server_dev);
	servers_online++;
}

/*
 * shm_connect - Creates a device and connects it into another device
 * [uint8_t * device_type] - Type of the device being created
 * [char * id] - ID AKA IP of the device we're going to connect to
 */
static shm_t * shm_connect(uint8_t device_type, char * id) {
	shm_t * shm_dev = malloc(sizeof(shm_t));
	shm_dev->dev_type = device_type;

	/* TODO: if device is a client and the id is non existant, throw an error*/

	/* otherwise, if it exists, connect into the server */

	/*else if, it's a server and the id doesn't exist, create one*/

	/*otherwise, if the id exists and the device is a server, throw an error*/


	/* TODO: add this device into a list */

	shm_dev->shm_key = malloc(strlen(id) + 1);
	strcpy(shm_dev->shm_key, id);
	shm_dev->shm = (char *) syscall_shm_obtain(id, &shm_dev->shm_size);

	return shm_dev;
}

/*
 * shm_connect - Disconnects and cleans up the device
 * [shm_t * dev] - Pointer to the device we're going to destroy
 */
static void shm_disconnect(shm_t * dev) {
	free(dev);
}

/*
 * shm_send - Send a packet to the shared memory "network". Both the server and client can do this.
 * [shm_t * dev] - shared memory node device
 * [void * packet] - actual data to send through the channel
 */
static void shm_send(shm_t * dev, void * packet) {

}

/*
 * shm_poll - Checks if the there's any request being made to the server. It's also non blocking
 * [shm_t * dev] - shared memory node device
 */
static void shm_poll(shm_t * dev) {

}

/*
 * shm_listen - Listens for requests by the clients. It's also blocking
 * [shm_t * dev] - shared memory node device
 */
static void shm_listen(shm_t * dev) {

}

void shm_ack(char * response) {
	//if(response) {
		char * s = malloc(5);
		strcpy(s, "hi");
		shm_cmd[1] = &s; /* the response will be found in shm_cmd + 1, the 1st char will be empty for acknowledge purposes */
	//}
	shm_cmd[0] = 0;
}

int lock = 0;
int main(int argc, char ** argv) {
	shm_network_init();

	while(1) {
		/* Listen for requests and handle them */
		spin_lock(&lock);
		switch(shm_cmd[0]) {
			case SHM_CMD_NEW_SERVER:
				shm_ack(NULL);
				goto ACK_DONE;
			case SHM_CMD_NEW_CLIENT:

				goto ACK_DONE;
				break;
			case SHM_CMD_SEND:
				/* Send packet into another process */
				break;
			case 0: /* No requests being done */
				goto ACK_DONE;
		}

		/* Clear out cmd flag */
		shm_ack(NULL);
		ACK_DONE:

		thread_sleep();
		spin_unlock(&lock);
	}
	return 0; /* The shared memory should never return */
}
