/*
 * shmon.h
 *
 *  Created on: 20 Sep 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_SHMON_H_
#define HELIOS_SRC_USR_INCLUDE_SHMON_H_

#include <stdint.h>
#include <shm.h>

/* Shell monitor server ip: */
#define SHMON_SERVER_IP "serv_shmon"

/* Formats for the client ids: */
#define SHMON_CLIENT_LOGIN_FORMAT "cllo_%d"
#define SHMON_CLIENT_SHELL_FORMAT "clsh_%d"

/* Message to send to processes to make them wake up */
#define SHMON_MSG_PROC_WAKE "wake"

/* Shared memory controllers */
#define MAX_MULTISHELL 10
#define MULTISHELL_STARTUP_COUNT 1

enum SHM_CTRL {
	SHMON_CTRL_GRAB_PID = 1,
	SHMON_CTRL_ADD_USR = 2,
	SHMON_CTRL_GET_USR = 3,
	SHMON_CTRL_REDIR = 4
};

typedef struct {
	uint8_t cmd;
	char cmd_dat[SHM_MAX_ID_SIZE * 2];
	char username[SHM_MAX_ID_SIZE];
} __attribute__((packed)) shmon_packet_t;

/* shmon.c */
extern void monitor_multishell();

/* shmon_api.c: */
extern uint32_t shmon_nextshell(char * switch_user, int parent_pid);
extern shm_t * init_shmon_api(char * client_id);
extern void send_wakeup_loginproc(int pid);
extern void shmon_send_user(char * username);
extern int shmon_get_user(char * username);

inline shmon_packet_t * create_shmon_packet(uint8_t cmd, char * cmd_dat, char * username) {
	shmon_packet_t * shmon_packet = malloc(sizeof(shmon_packet_t));
	shmon_packet->cmd = cmd;
	if(cmd_dat) {
		int cmd_dat_len = strlen(cmd_dat) + 1;
		memcpy(shmon_packet->cmd_dat, cmd_dat, cmd_dat_len >= SHM_MAX_ID_SIZE * 2 ? SHM_MAX_ID_SIZE * 2 : cmd_dat_len);
	}
	if(username) {
		int username_len = strlen(username) + 1;
		memcpy(shmon_packet->username, username, username_len >= SHM_MAX_ID_SIZE ? SHM_MAX_ID_SIZE : username_len);
	}
	return shmon_packet;
}

#endif /* HELIOS_SRC_USR_INCLUDE_SHMON_H_ */
