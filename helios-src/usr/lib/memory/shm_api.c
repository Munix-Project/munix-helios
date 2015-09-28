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

/* Size of channel used between any process and the shared memory manager */
size_t shman_size = 0;

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
		shman_send(shm_dev->dev_type + 3, shm_dev, sizeof(shm_t), NULL);
		free(shm_dev);
	}
}

/*
 * shm_send - Sends a command and packet to the shared memory manager.
 * 	The commands are:
 * 		* 1 - Create a new server
 * 		* 2 - Create a new client
 * 		* 3 - Destroy server
 * 		* 4 - Destroy client
		* 5 - Send packet to server/client
 * 		* 6 - Server listen for packet reception
 * 	[char cmd] - command to send
 * 	[char * packet] - packet/data to send
 * 	[char packet_size] - size of the struct of the packet (e.g. sizeof(shm_t), or sizeof(shm_packet_t). Note: don't use sizeof(shm_t*) with the pointer)
 *  [shm_t * dev_for_ack] - Device that will be used to filter the ack received. In other words, the ack will only be accepted if it is inteded for this device
 */
void * shman_send(char cmd, void * packet, uint8_t packet_size, shm_t * dev_for_ack) {
	if(!is_shman_init)
		shman_init();
	if(packet) { /* Send packet to the shared memory manager */
		/* Copy whatever the response variable points to to the shared memory with offset of 1 byte */
		shm_manager[1] = packet_size; /* second byte contains the size of the response */
		memcpy(shm_manager + 2, packet, packet_size); /* the third byte contains the start of the response data */
	}

	shm_manager[0] = cmd;
	if(dev_for_ack)
		shm_wait_for_ack_withdev(shm_manager, dev_for_ack); /* fetch this ack only if its for us */
	else
		shm_wait_for_ack(shm_manager); /* we're fetching an ack that was broadcasted for everyone */
	if(!shm_manager[1] && shm_manager[2])
		return shm_manager[2]; /* Oops, this result is invalid. Return the error number */
	else
		return get_packet_ptr(shm_manager); /* grab shm manager's acknowledge result, if there is one */
}

/* Function wrappers: */
shm_t * shman_create_server(char * server_id) {
	shm_t * server_from_shm = (shm_t*)shman_send(SHM_CMD_NEW_SERVER, server_id, strlen(server_id), NULL);
	if(SHM_IS_ERR(server_from_shm)) {
		/* We got an invalid response. Carry it to the process application */
		return server_from_shm;
	} else {
		shm_t * new_server = malloc(sizeof(shm_t));
		memcpy(new_server, server_from_shm, sizeof(shm_t));
		return new_server;
	}
}

shm_t * shman_create_client(char * server_id, char * client_id) {
	shm_id_unique_t * clientname = malloc(sizeof(shm_id_unique_t));
	strcpy(clientname->unique_id, client_id);
	strcpy(clientname->server_id, server_id);

	shm_t * client_from_shm = (shm_t*)shman_send(SHM_CMD_NEW_CLIENT, clientname, sizeof(shm_id_unique_t), NULL);
	if(SHM_IS_ERR(client_from_shm)) {
		/* We got an invalid response. Carry it to the process application */
		return client_from_shm;
	} else {
		shm_t * new_client = malloc(sizeof(shm_t));
		memcpy(new_client, client_from_shm, sizeof(shm_t));
		return new_client;
	}
}

/*
 * shman_send - Sends packet to server and receives response back (or not)
 * [shm_packet_t * packet] - Packet to be sent to the destination node
 */
shm_packet_t * shman_send_to_network(shm_t * dev, shm_packet_t * packet) {
	if(!is_shman_init) return SHM_ERR_NOINIT;

	/* This will prevent other processes from sending packets from clients that are not theirs */
	if(strcmp(dev->shm_key, packet->to))
		return SHM_ERR_NOAUTH;

	/* Must use a timeout on this. Sending packets needs a timeout, listening might not need one. */
	shm_packet_t * server_response =
			(shm_packet_t *) shman_send(SHM_CMD_SEND, packet, sizeof(shm_packet_t), dev);
	return server_response;
}

/*
 * shman_server_listen - Tells the Shared Memory Manager we're going to start listening for packages. Works for the server only.
 * [shm_t * server] - Server pointer
 * [shman_packet_cback callback] - Function callback that will be called every time a packet is received
 */
int shman_server_listen(shm_t * server, shman_packet_cback callback) {
	if(!is_shman_init) return SHM_ERR_NOINIT;
	if(server->dev_type == SHM_DEV_CLIENT) return SHM_ERR_WRONGDEV;

	/* Tell SHMAN we're going to start listening for requests */
	shm_packet_t * shman_serv_response =
			(shm_packet_t *)shman_send(SHM_CMD_SERVER_LISTEN, server, sizeof(shm_t), server); /* We don't want to receive anything. */

	/* We're now listening. We should wait for acks that are for us, instead of
	 * reacting to any ack that is broadcast from SHMAN */

	while(1) {
		char * cback_ret_dat;
		if((cback_ret_dat = callback(shman_serv_response))[0] == SHM_RET_SERV_WANTSQUIT)
			break; /* callback wants server to stop listening */

		/* send confirmation back to the shared memory manager with the callback's result */
		shman_send(SHM_CMD_SEND,
				create_packet(server, server->unique_id, SHM_DEV_SERVER, cback_ret_dat, SHM_MAX_PACKET_DAT_SIZE),
				sizeof(shm_packet_t),
				NULL
		);

		free(cback_ret_dat);

		/* we're already listening. just wait for more callback requests for us */
		shm_wait_for_ack_withdev(shm_manager, server);
	}

	return 0;
}
