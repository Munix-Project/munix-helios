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

#define POLL_THREAD_SLEEP_USEC 10000

/* Shared memory key used to communicate with the SHM manager */
#define SHM_RESKEY_SHMAN_CMD "shman_cmd"

enum shm_device_type {
	SHM_DEV_CLIENT, /* Unidirectional, only makes requests and reads from server */
	SHM_DEV_SERVER, /* Bidirectional. It reads the client and answers him */
};

enum shm_device_status {
	SHM_STAT_ONLINE, /* The server exists and/or the client is connected */
	SHM_STAT_OFFLINE /* The server is offline */
};

enum shm_cmd {
	SHM_CMD_NEW_SERVER = 1,
	SHM_CMD_NEW_CLIENT = 2,
	SHM_CMD_SEND = 3
};

typedef struct {
	uint8_t dev_type;
	uint8_t dev_status;
	size_t shm_size;
	void * shm;
	char * shm_key;
} shm_t;

/* Shared memory manager API functions: */
void * shm_send_cmd(char cmd);
void * shm_send_data_and_wait(void * packet, shm_t * dev);
void * shm_server_listen(shm_t * dev);

#endif /* HELIOS_SRC_USR_INCLUDE_SHM_H_ */
