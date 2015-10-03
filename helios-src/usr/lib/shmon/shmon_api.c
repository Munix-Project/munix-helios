/*
 * shmon_api.c
 *
 *  Created on: 22 Sep 2015
 *      Author: miguel
 */

#include <shmon.h>
#include <stdio.h>
#include <unistd.h>
#include <hashmap.h>
#include <list.h>
#include <sys/wait.h>
#include <spinlock.h>
#include <string.h>
#include <stdlib.h>
#include <syscall.h>

static shm_t * shmon_client;
static uint8_t is_shmon_client_init;

/*
 * init_shmon_api - Initializes the API for the Shell Monitor
 * [char * client_id] - ID of the client that will connect into the shell monitor's server
 */
shm_t * init_shmon_api(char * client_id) {
	if(!is_shmon_client_init) {
		shmon_client = shman_create_client(SHMON_SERVER_IP, client_id);
		is_shmon_client_init = 1;
		return shmon_client;
	}
	return NULL;
}

/*
 * shmon_get_user - Fetches a pid from the Shell Monitor using an associated username
 * [char * username] - Self explanatory
 */
int shmon_get_user(char * username) {
	int ret_pid = -1;
	shmon_packet_t * spacket = create_shmon_packet(SHMON_CTRL_GET_USR, NULL, username);
	shm_packet_t * shmon_response =
		shman_send_to_network(
			shmon_client,
			create_packet(shmon_client, SHMON_SERVER_IP, SHM_DEV_SERVER, spacket, sizeof(shmon_packet_t))
		);
	ret_pid = atoi(shmon_response->dat);
	free(shmon_response);
	free(spacket);
	return ret_pid;
}

/*
 * shmon_send_user - Adds a new user to the shell monitor's hashmap
 * [char * username] - String to send to the shmon (shell monitor)
 */
void shmon_send_user(char * username) {
	char pid_str[5];
	sprintf(pid_str, "%d", getpid());
	shmon_packet_t * spacket = create_shmon_packet(SHMON_CTRL_ADD_USR, pid_str, username);
	shman_send_to_network(
		shmon_client,
		create_packet(shmon_client, SHMON_SERVER_IP, SHM_DEV_SERVER, spacket, sizeof(shmon_packet_t))
	);
	free(spacket);
}

/*
 * send_wakeup_loginproc - Sends a message to the login process through shared memory
 * In this case, we're using it to allow the login process to wake up and continue
 * [int pid] - Destination Process ID
 */
void send_wakeup_loginproc(int pid) {
	if(!is_shmon_client_init) return;

	char pid_str[5];
	sprintf(pid_str, "%d", pid);
	shmon_packet_t * shmon_packet = create_shmon_packet(SHMON_CTRL_REDIR, pid_str, NULL);
	shman_send_to_network(
			shmon_client,
			create_packet(shmon_client, SHMON_SERVER_IP, SHM_DEV_SERVER, shmon_packet, sizeof(shmon_packet_t))
	);
	free(shmon_packet);
}

/*
 * shmon_nextshell - Grabs pid from the next shell counting from where the user is currently logged in
 * [char * switch_user] - To what user we're switching to
 * [int parent_pid] -
 */
uint32_t shmon_nextshell(char * switch_user, int parent_pid) {
	shm_packet_t * shmon_response = NULL;

	if(switch_user) {
		/* Send switch_user to shmon, and wait for pid to be set */
		shmon_packet_t * spacket = create_shmon_packet(SHMON_CTRL_GRAB_PID, NULL, switch_user);
		shmon_response =
			shman_send_to_network(
				shmon_client,
				create_packet(shmon_client, SHMON_SERVER_IP, SHM_DEV_SERVER, spacket, sizeof(shmon_packet_t))
			);
		free(spacket);
	}

	/* At this moment, we got ourselves the right PID for the next shell */
	int nextshell_pid_int = atoi(shmon_response->dat);

	free(shmon_response);
	return nextshell_pid_int == parent_pid ? -1 : nextshell_pid_int;
}
