/*
 * shmon.h
 *
 *  Created on: 20 Sep 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_SHMON_H_
#define HELIOS_SRC_USR_INCLUDE_SHMON_H_

#include <stdint.h>

/* Shared memory controllers */
#define MAX_MULTISHELL 10
#define MULTISHELL_STARTUP_COUNT 1
#define SHM_SHELLMON_IN "shm_shellmon_in"
#define SHM_SHELLMON_OUT "shm_shellmon_out"
#define SHM_CTRL_GRAB_PID '1'
#define SHM_CTRL_ADD_USR '2'
#define SHM_CTRL_GET_USR '3'
#define SHM_SHELLMON_KILLITSELF "shm_shellmon_kill_login:"

extern void monitor_multishell();

extern uint32_t shmon_nextshell(char * switch_user, int parent_pid);
extern void send_kill_msg_login(int pid);

extern int listen_to_shm();
extern void init_shm();
extern void send_kill_msg_login(int pid);
extern void shmon_send_user(char * username);
extern int shmon_get_user(char * username);

#endif /* HELIOS_SRC_USR_INCLUDE_SHMON_H_ */
