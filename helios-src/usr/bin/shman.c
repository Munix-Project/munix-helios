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
#include <syscall.h>
#include <spinlock.h>
#include <pthread_os.h>

/* Size of channel used between any process and the shared memory manager */
size_t shman_size = SHMAN_SIZE;
static char * shm_cmd; /* Shared memory used to execute commands and receive data which the commands might need */

static hashmap_t * shm_network; /* contains the server's ids as keys and a list with their clients */
static int servers_online = 0;

/*
 * shm_network_init - Shared Memory Manager constructor
 */
static void shm_network_init() {
	shm_network = hashmap_create(1);
	shm_cmd = (char*) syscall_shm_obtain(SHM_RESKEY_SHMAN_CMD, &shman_size);
}

/*
 * shm_network_stopall - Shared Memory Manager destructor
 */
static void shm_network_stopall() {
	hashmap_free(shm_network);
	free(shm_network);
	syscall_shm_release(shm_cmd);
}

static void add_server_to_network(shm_t * server_dev) {
	list_t * network_list = list_create();
	list_insert(network_list, server_dev); /* the first node of this network list will always be the server itself */
	hashmap_set(shm_network, server_dev->shm_key, network_list);
	servers_online++;
}

shm_t * get_server_from_network(char * server_id) {
	list_t * shm_node_list = (list_t*)hashmap_get(shm_network, server_id);
	shm_t * server = (shm_t*)shm_node_list->head->value;
	return server;
}

/*
 * shm_connect - Creates a device and connects it into another device (unless the device being created is a server)
 * [uint8_t * device_type] - Type of the device being created
 * [char * id] - ID AKA IP of the device we're going to connect to
 */

static shm_t * shm_connect(uint8_t device_type, char * id, char * unique_id) {
	/* RULES: if device is a client and the id is non existant, throw an error*/
	/* otherwise, if it exists, connect into the server */
	/* else if it's a server and the id doesn't exist, create one*/
	/* otherwise, if the id exists and the device is a server, throw an error*/
	if(device_type == SHM_DEV_CLIENT && (!hashmap_contains(shm_network, id) || !servers_online)) {
		/* Can't connect to that server. It is offline / doesn't exist */
		return (shm_t*)SHM_ERR_SERV_NOEXIST;
	} else if(device_type == SHM_DEV_SERVER && hashmap_contains(shm_network, id)) {
		/* That server is already online. We don't want to have ID conflicts */
		return (shm_t*)SHM_ERR_SERV_ALREADYON;
	}

	/*
	 * Before we even try to connect as client, we must know if the server we're
	 * connecting to is also online
	 */
	if(device_type == SHM_DEV_CLIENT) {
		if(get_server_from_network(id)->dev_status == SHM_STAT_OFFLINE) {
			/* Oops, the server is offline */
			return (shm_t*)SHM_ERR_SERV_OFFLINE;
		}
	}

	/* All seems good. We're continuing the function */

	shm_t * shm_new_dev = malloc(sizeof(shm_t));

	shm_new_dev->dev_type = device_type; /* Set its type */
	shm_new_dev->dev_status = SHM_STAT_ONLINE; /* Set device as online */

	strcpy(shm_new_dev->shm_key, id); /* Set its ID so we know what server we're called/what server we're connected to */
	strcpy(shm_new_dev->unique_id, unique_id);
	shm_new_dev->shm = (uint8_t*)syscall_shm_obtain(id, &shman_size);
	memset(shm_new_dev->shm, 0, shman_size);

	/* Add this device into a hashmap with a list (which indicates the nodes connected to the hash) as value and ID as key */
	if(device_type == SHM_DEV_SERVER)
		add_server_to_network(shm_new_dev);

	return shm_new_dev;
}

/*
 * shm_disconnect - Disconnects and cleans up the device
 * [shm_t * dev] - Pointer to the device we're going to destroy
 */
static void shm_disconnect(shm_t * dev) {
	if(dev->dev_type == SHM_DEV_SERVER) {
		hashmap_remove(shm_network, dev->shm_key);
		servers_online--;
	}
}

/*
 * shm_ack - Broadcasts acknowledgement to the network
 * [void * response] - packet to send to the server/client or to process who executed command on the Shared Memory Manager
 * [char response_size] - Size in bytes of the packet being sent
 */
static void shm_ack(void * response, char response_size) {
	if(response) {
		/* Copy whatever the response variable points to to the shared memory with offset of 1 byte */
		shm_cmd[1] = response_size; /* second byte contains the size of the response */
		if(!response_size)
			response_size++; /* this means we might have found an error */
		memcpy(shm_cmd + 2, response, response_size); /* the third byte contains the start of the response data */
	}
	shm_cmd[0] = 0; /* the first byte must be used to signal the acknowledgement of the request */
}

/*
 * send_confirmation - Send acknowledgement WITH data to the process who requested data/executed a command
 * [void * dat_ptr] - Pointer to the data to be sent
 * [char confirmation_size] - Size of the structure of the pointer
 */
void send_confirmation(void * dat_ptr, char confirmation_size) {
	shm_ack(dat_ptr, confirmation_size);
	free(dat_ptr);
}

static void shm_send_to_server(shm_packet_t * packet) {
	shm_t * server = get_server_from_network(packet->to);
	server->shm[1] = sizeof(shm_packet_t); /* tells the server the size of the packet */
	memcpy(server->shm + 2, packet, sizeof(shm_packet_t)); /* copy packet to server */
	server->shm[0] = 1; /* indicates the server there's a new package */
	shm_ack(NULL, 0);
}

/*
 * send_err - Sends error code to the process who made the request
 * [char errcode] - self explanatory
 */
void send_err(char errcode) {
	char * errcode_buff = malloc(1);
	errcode_buff[0] = errcode;
	shm_ack(errcode_buff, 0);
	free(errcode_buff);
}

/*
 * shm_poll - Checks if the there's any request being made to the server. It's also non blocking
 * [shm_t * dev] - shared memory node device
 */
static int shm_poll(char * shm) {
	return shm[0];
}

/*
 * shm_listen - Listens for requests by the clients. It's also blocking
 * [shm_t * dev] - shared memory node device
 */
int lock = 0;
char * shm_old = NULL;
static void server_wait_for_packet(shm_t * dev) {
	if(!shm_old) shm_old = dev->shm;
	int poll_ret = 0;
	spin_lock(&lock);
	while(!poll_ret) {
		thread_sleep();
		/* This is a fix for a really really really really annoying bug where the address of dev->shm magically changes
		 * by doing 'memcpy' on another pointer on the file shm_api.c line 60. I really don't know how to fix this and it pisses me off.
		 * If you have any idea about this please tell me :) */
		if(dev->shm != shm_old) dev->shm = shm_old; /* wut?... ffs... */
		else shm_old = dev->shm;

		poll_ret = shm_poll(dev->shm);
	}
	spin_unlock(&lock);
}

void * shm_server_listen(void * dev_ptr) {
	shm_t * dev = (shm_t*) dev_ptr;
	if(dev->dev_type != SHM_DEV_SERVER) {
		pthread_exit(1);
		return NULL; /* Don't even try to do this if you're a client */
	}

	/* And now we wait */
	while(1) {
		server_wait_for_packet(dev);

		/* Someone broadcasted an ack! Is it for us?? */

		shm_packet_t * packet = (shm_packet_t*)get_packet_ptr(dev->shm);

		/* Let's see... */
		if(SHM_PACKET_IS_IT_FOR_US(packet, dev)) { /* Prevent the server from fetching packets from itself */
			/* It's for us! But is the packet from someone we should trust?.... */
			if(packet->to_type == SHM_DEV_CLIENT) {
				/* The packet says this is supposed to be received by a client.... That's impossible.
				 * Just kill this packet already so the client knows he screwed up and it isn't going to mess up
				 * other servers. */
				/* Build packet and send it back */
				send_confirmation(
						create_packet(dev, dev->shm_key, SHM_DEV_CLIENT, SHM_PACK_BADPACKET, 1),
						sizeof(shm_packet_t)
				);
				continue;
			}

			char client_name[SHM_MAX_ID_SIZE];
			strcpy(client_name, packet->to);

			/* Looking good. */

			/* Send packet for the server's callback */
			send_confirmation(packet, sizeof(shm_packet_t));

			/* And now listen for the callback response */
			server_wait_for_packet(dev);

			printf("callback responded!"); fflush(stdout);

			/* Finally, send callback's result back directly to the client */
			shm_packet_t * cback_packet = (shm_packet_t*)get_packet_ptr(dev->shm);
			strcpy(cback_packet->to, client_name);
			send_confirmation(cback_packet, sizeof(shm_packet_t));

			/* Good, all done! */
		}

		/* else it's not for us... We just hope someone will receive it and send ack to the client,
		 * otherwise we might just stay here forever. We gotta find a fix for this. (Unless the client uses timeouts...) */
	}

	pthread_exit(0);
	return 0;
}

int main(int argc, char ** argv) {
	shm_network_init();

	while(1) {
		/* Listen for requests and handle them */
		switch(shm_cmd[0]) {
			case SHM_CMD_NEW_SERVER: {
				/* get_packet_ptr is expected to return the start of the string
				 * that must be passed in while communicating with this process.
				 * The same applies to the next case on this switch */
				shm_t * new_server = shm_connect(SHM_DEV_SERVER, get_packet_ptr(shm_cmd), get_packet_ptr(shm_cmd));
				if(SHM_IS_ERR(new_server)) {
					/* The server creation failed. Send error to whoever requested the server */
					send_err(new_server);
				} else {
					/* The server is good */
					send_confirmation(new_server, sizeof(shm_t));
				}
				break;
			}
			case SHM_CMD_NEW_CLIENT: {
				shm_id_unique_t * clientname = (shm_id_unique_t *)get_packet_ptr(shm_cmd);
				shm_t * new_client = shm_connect(SHM_DEV_CLIENT, clientname->server_id, clientname->unique_id);
				if(SHM_IS_ERR(new_client)) {
					/* The client creation failed. Send error to whoever requested the client */
					send_err(new_client);
				} else {
					/* The client is good */
					send_confirmation(new_client, sizeof(shm_t));
				}
				break;
			}
			case SHM_CMD_DESTROY_SERVER:
				shm_disconnect((shm_t*)get_packet_ptr(shm_cmd));
				shm_ack(NULL, 0);
				break;
			case SHM_CMD_DESTROY_CLIENT:
				shm_disconnect((shm_t*)get_packet_ptr(shm_cmd));
				shm_ack(NULL, 0);
				break;
			case SHM_CMD_SEND: {
				/* Send packet directly from client to server: */
				shm_send_to_server((shm_packet_t*)get_packet_ptr(shm_cmd));
				break;
			}
			case SHM_CMD_SERVER_LISTEN: {
				pthread_t listen_thread;
				pthread_create(&listen_thread, NULL, shm_server_listen, get_packet_ptr(shm_cmd));
				shm_ack(NULL, 0); /* broadcast ack but indicate no destination nor error code */
				break;
			}
			case 0: /* No requests being done */
			default:
				/* Clear out cmd flag */
				shm_ack(NULL, 0);
		}
		thread_sleep();
	}

	shm_network_stopall();
	return 0; /* The shared memory should never return */
}
