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

char shm_key[30];
int shm_lock = 0;
char * shm;

size_t s = 24;

/* TODO: Implement a proper general purpose shared memory library. Not this API which only works for logging in and out */

/* These next functions are used for login.c: */

int shmon_get_user(char * username) {
	int ret_pid = -1;

	size_t s = 0;
	char * shm = (char*)syscall_shm_obtain(SHM_SHELLMON_OUT, &s);
	char cmd[20];
	sprintf(cmd, "-%c:%s\n", SHM_CTRL_GET_USR, username);
	strcpy(shm, cmd); /* send the message to 'terminal' process */

	while(shm[0]) usleep(10000); /* wait for acknowledge */

	shm = (char*)syscall_shm_obtain(SHM_SHELLMON_IN, &s);
	char * nextshell_pid = (char*) (strchr(shm, ':') + 1);
	ret_pid = atoi(nextshell_pid);

	return ret_pid;
}

void shmon_send_user(char * username) {
	size_t s = 0;

	char * shm = (char*)syscall_shm_obtain(SHM_SHELLMON_OUT, &s);
	char cmd[20];
	sprintf(cmd, "-%c:%s\n%d", SHM_CTRL_ADD_USR, username, getpid());
	strcpy(shm, cmd);

	while(shm[0]) usleep(10000); /* wait for acknowledge */
}

void shm_wait(char * shm_key) {
	size_t s = 0;
	char * shm = (char*)syscall_shm_obtain(shm_key, &s);
	while(shm[0]) usleep(10000);
}

void shman_send(char * shm_key, char * packet) {
	size_t s = 0;
	strcpy((char*)syscall_shm_obtain(shm_key, &s), packet);
}

void shm_send_and_wait(char * shm_key, char * packet) {
	shman_send(shm_key, packet);
	shm_wait(shm_key);
}

/*
 * Sends a message to the login process through shared memory
 * In this case, we're using it to allow the login process to wake up and continue
 */
void send_kill_msg_login(int pid) {
	size_t s = 1;
	char shm_key[30];
	memset(shm_key, 0, 30);
	strcat(shm_key, SHM_SHELLMON_KILLITSELF);
	char pid_str[5];
	memset(pid_str, 0, 5);
	sprintf(pid_str, "%d", pid);
	strcat(shm_key, pid_str);
	char * shm = (char*) syscall_shm_obtain(shm_key, &s);

	shm[0] = 1; /* just a flag */
	shm_wait(shm_key);
}

char * init_shm() {
	size_t s = 1;
	memset(shm_key, 0, 30);
	strcat(shm_key, SHM_SHELLMON_KILLITSELF);
	char pid_str[5];
	memset(pid_str, 0, 5);
	sprintf(pid_str, "%d", getpid());
	strcat(shm_key, pid_str);

	shm = (char*) syscall_shm_obtain(shm_key, &s);
	memset(shm, 0, 5);
	return shm;
}

/*
 * Listens to the shared memory flag without blocking the thread
 */
int listen_to_shm(char * shm_key, char * shm) {
	size_t s  = 0;
	int ret = 0;
	spin_lock(&shm_lock);
	if(shm[0]){
		shm[0] = 0;
		ret = 1;
	}
	spin_unlock(&shm_lock);
	return ret;
}

/*
 * Listens to the shared memory flag and also blocks the thread
 */
void listen_to_shm_block(char * shm_key, char * shm) {
	while(!listen_to_shm(shm_key, shm)) usleep(10000);
}

char * shm_read(char * shm_key) {
	return (char*)syscall_shm_obtain(shm_key, &s);
}

/* These next functions are used for sh.c: */

/*
 * Grabs pid from the next shell counting from where the user is currently logged in
 */
uint32_t shmon_nextshell(char * switch_user, int parent_pid) {
	if(switch_user) {
		/* Send switch_user to shmon, and wait for pid to be set */
		char msg[20];
		sprintf(msg, "-%c:%s\n", SHM_CTRL_GRAB_PID, switch_user);
		/* Send packet to shared memory in order to allow the shmon component to output its data.
		 * After sending, wait for monitor to read and clear out the buffer */
		shm_send_and_wait(SHM_SHELLMON_OUT, msg);
	}

	/* At this moment, we got ourselves the right PID for the next shell */
	shm = shm_read(SHM_SHELLMON_IN);
	printf("reading pid: %s\n", shm);
	fflush(stdout);
	char * nextshell_pid = (char*) (strchr(shm, ':') + 1); /* grab pid */
	int nextshell_pid_int = atoi(nextshell_pid);

	return nextshell_pid_int == parent_pid ? -1 : nextshell_pid_int;
}
