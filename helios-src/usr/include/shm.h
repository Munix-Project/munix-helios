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

#define SHMAN_SIZE 128
extern size_t shman_size;

#define POLL_THREAD_SLEEP_USEC 10000

/* Shared memory key used to communicate with the SHM manager */
#define SHM_RESKEY_SHMAN_CMD "shman_cmd"

/*
 * Describes the type of device used by the Shared Memory Manager
 */
enum shm_device_type {
	SHM_DEV_SERVER, /* Bidirectional. It reads the client and answers him */
	SHM_DEV_CLIENT, /* Unidirectional, only makes requests and reads from server */
};

enum shm_ret_error {
	SHM_ERR_SERV_NOEXIST = 1,
	SHM_ERR_SERV_ALREADYON  = 2,
	SHM_ERR_SERV_OFFLINE = 3,
	SHM_NOERR = 4
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

/* Shared memory device structure: */
typedef struct {
	uint8_t dev_type;
	uint8_t dev_status;
	size_t shm_size;
	void * shm;
	char shm_key[16];
} shm_t;

/* Shared memory manager API functions: */
char * shman_start();
void shman_stop(shm_t * shm_dev);
void * shman_send(char cmd, void * packet, char packet_size);

inline void thread_sleep() {
	usleep(POLL_THREAD_SLEEP_USEC);
}

inline void shm_wait_for_ack(char * shm) {
	while(shm[0]) thread_sleep();
}

/* API Wrapper functions: */
shm_t * shman_create_server(char * server_id);
shm_t * shman_create_client(char * server_id);

#endif /* HELIOS_SRC_USR_INCLUDE_SHM_H_ */
