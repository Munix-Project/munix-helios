/*
 * shm.h
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_SHM_H_
#define HELIOS_SRC_USR_INCLUDE_SHM_H_

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* Shared Memory Manager channel size */
#define SHMAN_SIZE 128
extern size_t shman_size;

/* Time in microseconds for the thread to wait for the polling */
#define POLL_THREAD_SLEEP_USEC 0

/* Shared memory key used to communicate with the SHM manager */
#define SHM_RESKEY_SHMAN_CMD "shman_cmd" /* This key is reserved and shall not be used by another process */

/* Max length of the string id's of the shm devices */
#define SHM_MAX_ID_SIZE 16

/* Size of the actual data packet */
#define SHM_MAX_PACKET_DAT_SIZE SHMAN_SIZE - (SHM_MAX_ID_SIZE * 2) - 3

#define SHM_IS_ERR(no) no < SHM_NOERR

/*
 * Describes the type of device used by the Shared Memory Manager
 */
enum shm_device_type {
	SHM_DEV_SERVER, /* Bidirectional. It reads the client and answers him */
	SHM_DEV_CLIENT, /* Unidirectional, only makes requests and reads from server */
};

/*
 * Error codes:
 */
enum shm_ret_error {
	SHM_ERR_SERV_NOEXIST = 1,
	SHM_ERR_SERV_ALREADYON  = 2,
	SHM_ERR_SERV_OFFLINE = 3,
	SHM_ERR_NOINIT = 4,
	SHM_ERR_INVDEV = 5,
	SHM_ERR_WRONGDEV = 6,
	SHM_NOERR = 7
};

/*
 * Return codes which are not errors:
 */
enum shm_ret_code {
	SHM_RET_SUCCESS = 8,
	SHM_RET_SERV_WANTSQUIT = 9 /* not necessarily an error... */
};

/*
 * Describes the status of the device used by the Shared Memory Manager
 */
enum shm_device_status {
	SHM_STAT_ONLINE, /* The server exists and/or the client is connected */
	SHM_STAT_OFFLINE /* The server/client is offline */
};

/*
 * The commands that can be sent to the Shared Memory Manager
 */
enum shm_cmd {
	SHM_CMD_NEW_SERVER = 1,
	SHM_CMD_NEW_CLIENT = 2,
	SHM_CMD_DESTROY_SERVER = 3,
	SHM_CMD_DESTROY_CLIENT = 4,
	SHM_CMD_SEND = 5,
	SHM_CMD_SERVER_LISTEN = 6,
};

/*
 * Simple bytes that are going to be used as flags for devices when they send data and might expect a structured
 * response but instead get a code because they either screwed up or are getting a warning instead of their data that they wanted.
 */
enum shm_packet_codes {
	SHM_PACK_BADPACKET = 1
};

/* Shared memory device structure: */
typedef struct {
	uint8_t dev_type;
	uint8_t dev_status;
	size_t shm_size;
	void * shm;
	char shm_key[SHM_MAX_ID_SIZE];
} shm_t;

/* Packet definition to be sent between processes. It is delivered by the Shared Memory Manager to the servers/clients */
typedef struct {
	uint8_t from_type;
	uint8_t to_type;
	char from[SHM_MAX_ID_SIZE];
	char to[SHM_MAX_ID_SIZE];
	char dat[SHM_MAX_PACKET_DAT_SIZE];
	uint8_t dat_size;
} shm_packet_t;

typedef int (*shman_packet_cback)(shm_packet_t*);

/* Shared memory manager API functions: */
char * shman_start();
void shman_stop(shm_t * shm_dev);
void * shman_send(char cmd, void * packet, char packet_size);

inline void thread_sleep() {
	usleep(POLL_THREAD_SLEEP_USEC);
}

/*
 * Waits for acknowledgement on the shared memory.
 * WARNING: This method checks for ack in broadcast.
 * It does not check the destination ack.
 */
inline void shm_wait_for_ack(char * shm) {
	while(shm[0]) thread_sleep();
}

/*
 * Retrieves the pointer to where the
 * data sent to this manager resides in
 */
inline char * get_packet_ptr(char * shm) {
	return &shm[2];
}

/*
 * create_packet - Creates a shared memory network packet using inputs from the application
 * [shm_t * our_device] - The device we're going to send this packet FROM
 * [char * to] - IP/ID of the destination device
 * [uint8_t to_dev_type] - Type of the destination device (server/client)
 * [void * message] - Pointer to the message struct. This better not exceed the whole packet size
 * [uint8_t message_size] - Size in bytes of the message struct / string
 */
inline shm_packet_t * create_packet(shm_t * our_device, char * to, uint8_t to_dev_type, void * message, uint8_t message_size) {
	shm_packet_t * packet = malloc(sizeof(shm_packet_t));

	strcpy(packet->to, to); /* IP/ID/Shared Memory Key of the server */
	strcpy(packet->from, our_device->shm_key);

	/* Actually copy the data */
	memcpy(packet->dat, message, message_size >= SHM_MAX_PACKET_DAT_SIZE ? SHM_MAX_PACKET_DAT_SIZE : message_size);
	packet->dat_size = message_size;

	packet->from_type = our_device->dev_type; /* Our device type */
	packet->to_type = to_dev_type; /* What type is the device we're sending this packet to? Server? Client? */

	return packet;
}

/* API Wrapper functions: */
shm_t * shman_create_server(char * server_id);
shm_t * shman_create_client(char * server_id);
int shman_server_listen(shm_t * server, shman_packet_cback callback);

#endif /* HELIOS_SRC_USR_INCLUDE_SHM_H_ */
