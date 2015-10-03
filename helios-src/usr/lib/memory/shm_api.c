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
#include <stdio.h>

/* Size of channel used between any process and the shared memory manager */
size_t shman_size = 0;

static char * shm_manager;
static uint8_t is_shman_init = 0;

/*
 * get_cmd_channel - Returns the pointer to the channel used for sending commands to the Shared Memory Manager
 */
static void * get_cmd_channel() {
	return (void*)syscall_shm_obtain(SHM_RESKEY_SHMAN_CMD, &shman_size);
}

/*
 * shman_copy_from_shm - Allocates a packet and copies the packet from the shared memory
 * [char * shm] - Shared Memory Pointer
 */
static shm_packet_t * shman_copy_from_shm(char * shm) {
	shm_packet_t * packet = malloc(sizeof(shm_packet_t));
	memcpy(packet, get_packet_ptr(shm), sizeof(shm_packet_t));
	return packet;
}

/*
 * shman_clear_channel - Clears entire shared memory buffer. This can also be used for acknowledging and clearing out the data after fetching it.
 * [shm_t * dev] - Shared Memory Device
 */
static void shman_clear_channel(shm_t * dev) {
	memset(dev->shm, 0, SHMAN_SIZE);
}

/*
 * avoid_collision - Prevents packets from being sent if the channel is already being used.
 * [char * shm] - Channel from which the packets will "flow through"
 */
static void avoid_collision(char * shm) {
	TIMEOUT_START();

	while(shm[0]) {
		thread_sleep();
		TIMEOUT_CHECK();
	}
}

/*
 * shman_start - Initializes the shared memory channel used to communicate with the shared memory manager
 */
char * shman_start() {
	while(!shm_manager) {
		/* If 'get_cmd_channel' returns 0, then it means the shared memory manager hasn't startup up yet.
		 * Keep polling the shared memory until it starts working and then it will return a proper address
		 * for the shm_manager variable */
		shm_manager = get_cmd_channel();
		usleep(POLL_THREAD_SLEEP_USEC);
	}

	is_shman_init = 1;
	return shm_manager;
}

/*
 * shman_stop - Disconnects a device from a network
 * [shm_t * shm_dev] - Shared Memory Device to disconnect
 */
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
		shman_start();

	/* Wait for the command channel to free up to prevent collisions */
	avoid_collision(shm_manager);

	if(packet) { /* Send packet to the shared memory manager */
		/* Copy whatever the response variable points to to the shared memory with offset of 1 byte */
		shm_manager[1] = packet_size; /* second byte contains the size of the response */
		memcpy(shm_manager + 2, packet, packet_size); /* the third byte contains the start of the response data */
	}

	shm_manager[0] = cmd;
	if(dev_for_ack)
		shm_wait_for_ack_withdev(dev_for_ack->shm, dev_for_ack); /* fetch this ack only if its for us */
	else
		shm_wait_for_ack(shm_manager); /* we're fetching an ack that was broadcasted for everyone */

	if(!shm_manager[1] && shm_manager[2]) {
		return shm_manager[2]; /* Oops, this result is invalid. Return the error number */
	} else {
		if(dev_for_ack)
			return get_packet_ptr(dev_for_ack->shm);
		else
			return get_packet_ptr(shm_manager); /* grab shm manager's acknowledge result, if there is one */
	}
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
		new_server->shm = (char*)syscall_shm_obtain(server_id, &shman_size);
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
		new_client->shm = (char*)syscall_shm_obtain(server_id, &shman_size);
		return new_client;
	}
}

/*
 * shman_send_to_network_clear - Sends packet to server and receives response back (or not)
 * NOTE: The uint8_t clear_buffer indicates whether or not we will clear the shm buffer after the data is sent
 * [shm_packet_t * packet] - Packet to be sent to the destination node
 * [uint8_t clear_buffer] - Flag for clearing the shm buffer
 */
shm_packet_t * shman_send_to_network_clear(shm_t * dev, shm_packet_t * packet, uint8_t clear_buffer) {
	if(!is_shman_init) return SHM_ERR_NOINIT;

	/* This will prevent other processes from sending packets from clients that are not theirs */
	if(strcmp(dev->shm_key, packet->to))
		return SHM_ERR_NOAUTH;

	/* Prevent collisions on the network */
	avoid_collision(dev->shm);

	/* Must use a timeout on this. Sending packets needs a timeout, listening might not need one. */
	shm_packet_t * server_response_ptr =
			(shm_packet_t *) shman_send(SHM_CMD_SEND, packet, sizeof(shm_packet_t), dev);

	/* Copy Shared Memory data into our own memory */
	shm_packet_t * server_response = malloc(sizeof(shm_packet_t));
	memcpy(server_response, server_response_ptr, sizeof(shm_packet_t));

	/* Clear out shared memory channel in its entirety */
	if(clear_buffer)
		shman_clear_channel(dev);

	/* Return data back to the client */
	return server_response;
}

/*
 * shman_send_to_network - Sends packet to network and receives response back (or not)
 * [shm_t * dev] - Device to send the packet FROM
 * [shm_packet_t * packet] - Packet to be sent to the destination node
 */
shm_packet_t * shman_send_to_network(shm_t * dev, shm_packet_t * packet) {
	return shman_send_to_network_clear(dev, packet, 1);
}

/*
 * server_wait_for_packet - Used for listening on the server side
 * [shm_t * dev] - Server device
 */
static void server_wait_for_packet(shm_t * dev) {
	while(!dev->shm[0])
		thread_sleep();
}

/*
 * shman_server_listen - Listens for packets and redirects them to the server callback. Works for servers only.
 * [shm_t * server] - Server pointer
 * [shman_packet_cback callback] - Function callback that will be called every time a packet is received
 */
int shman_server_listen(shm_t * server, shman_packet_cback callback) {
	if(server->dev_type != SHM_DEV_SERVER)
		return SHM_ERR_WRONGDEV; /* Don't even try to do this if you're a client */

	while(1) {
		/* Listen for packets... */
		server_wait_for_packet(server);

		/* We got a packet! */

		/* Grab client's packet from the shared memory: */
		shm_packet_t * client_request = malloc(sizeof(shm_packet_t));
		memcpy(client_request, get_packet_ptr(server->shm), sizeof(shm_packet_t));

		/* Call server's callback to process the client's request */
		char* client_response_dat = callback(client_request);
		shm_packet_t * clients_response =
				create_packet(server, client_request->from, SHM_DEV_CLIENT, client_response_dat, SHM_MAX_PACKET_DAT_SIZE);

		/* Send callback's return data back to the client */
		server->shm[1] = sizeof(shm_packet_t);
		memcpy(server->shm + 2, clients_response, sizeof(shm_packet_t));
		server->shm[0] = 0;

		free(client_response_dat);
		free(clients_response);
	}

	return 0;
}

/*
 * client_send - This function is to be used only by the client where he sends basic messages to the server
 * [shm_t * client] - Client's shared memory device
 * [void * message] - Pointer to the message being sent
 * [uint8_t message_size_bytes] - Size in, you guessed it, bytes, of the message being sent
 */
shm_packet_t * client_send(shm_t * client, void * message, uint8_t message_size_bytes) {
	if(client->dev_type != SHM_DEV_CLIENT)
		return SHM_ERR_WRONGDEV;

	shm_packet_t * clients_packet = create_packet(client, client->shm_key, SHM_DEV_SERVER, message, message_size_bytes);
	shm_packet_t * servers_packet = shman_send_to_network(client, clients_packet);
	return servers_packet;
}

/*
 * client_has_packet - Checks (but doesn't wait) if the user has a new packet. If it has, it returns it, otherwise returns NULL
 * [shm_t * client] - Shared Memory Client Device
 */
shm_packet_t * client_has_packet(shm_t * client) {
	shm_packet_t * packet = NULL;
	if(client->shm[0]) {
		packet = (shm_packet_t*)get_packet_ptr(client->shm);
		if(SHM_PACKET_IS_IT_FOR_US(packet, client)) {
			packet = shman_copy_from_shm(client->shm); /* fetch packet */
			shman_clear_channel(client); /* acknowledge it and clear it out */
		} else {
			packet = NULL;
		}
	}
	return packet;
}

/*
 * client_has_packet_broadcast_deviceless - Really dangerous function which indicates if there is
 * data on the shared memory. It does not support shared memory devices nor packets.
 * This is currently used on both login.c and sh.c for 'client-client' communication
 * [char * shm] - Shared Memory Channel
 */
int client_has_packet_broadcast_deviceless(char * shm) {
	if(shm[0]) {
		shm[0] = 0; /* clear out flag. we now know we had something there */
		return 1;
	}
	return 0;
}
