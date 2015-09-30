/*
 * shm_client.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#include <shm.h>
#include <stdio.h>
#include <pthread_os.h>

char msg[16] = "hi!";
shm_t * client;
int c = 0;
void * f(void * b) {
	shm_packet_t * server_packet = client_send(client, msg, strlen(msg));

	printf("Server's Data: %s\n", server_packet->dat);
	fflush(stdout);
	msg[0]++;
	c--;
	pthread_exit(0);
	return 0;
}

int main(int argc, char ** argv) {
	client = shman_create_client("serv_1", "client_1");
	for(int i=0;i<10;i++) {
		c++;
		pthread_t p;
		pthread_create(&p, NULL, f, NULL);
	}
	while(c);
	shman_stop(client);

	return 0;
}
