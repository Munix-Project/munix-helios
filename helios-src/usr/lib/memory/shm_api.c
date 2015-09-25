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
#include <unistd.h>
#include <spinlock.h>

static void thread_sleep() {
	usleep(POLL_THREAD_SLEEP_USEC);
}

static void * get_channel(char * channel) {
	size_t size = 2;
	return (void*)syscall_shm_obtain(channel, &size);
}

static void * get_cmd_channel() {
	return get_channel(SHM_RESKEY_SHMAN_CMD);
}

int lock = 0;
void shm_wait_for_ack(char * shm) {
	spin_lock(&lock);
	while(shm[0]) thread_sleep();
	spin_unlock(&lock);
}

/*
 * shm_send_cmd - Send a command to the shared memory manager.
 * 	The commands are:
 * 		* 1 - Create a new server
 * 		* 2 - Create a new client
 * 		* 3 - Send packet to server
 * 	[char cmd] - command to send
 */
#include <stdio.h>
void * shm_send_cmd(char cmd) {
	char * shm = get_cmd_channel();
	shm[0] = cmd;

	shm_wait_for_ack(shm);
	char * str = (char*) shm[1];

	printf("Ack done! Return: 0x%x\n", *str);
	fflush(stdout);
	return shm; /* return result, if there is one */
}

void * shm_send_data_and_wait(void * packet, shm_t * dev) {
	char * shm = get_channel(dev->shm_key);
	shm[0] = packet; /* set shared memory data. could be a pointer or just a character flag */
	shm_wait_for_ack(shm); /* wait for the shared memory manager to acknowledge the request and respond (note that the response is not necessary) */
	return shm; /* return result, if there is one */
}

void * shm_server_listen(shm_t * dev) {

}
