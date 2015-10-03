/*
 * shmon.c
 *
 *  Created on: 22 Sep 2015
 *      Author: miguel
 *
 *  Creates multiple shell sessions, manages and stops them
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
#include <shm.h>
#include <pthread_os.h>

/* from terminal.c: */
extern int fd_slave;
extern volatile int exit_application;

/* Shell monitor data: */
static int shells_forked = 0;
hashmap_t * shellpid_hash;
hashmap_t * multishell_sessions;

/* We're using the shell monitor as a server which will serve multiple clients */
shm_t * shmon_server;

/*
 * grab_io - Redirects input and output into the file descriptor
 * [int file_descriptor] - File descriptor, obviously
 */
static void grab_io(int file_descriptor) {
	for(int i=0;i < 3;i++)
		dup2(file_descriptor, STDIN_FILENO + i);
}

/*
 * spawn_shell - Creates a new shell session
 * [int forkno] - Index of the shell session
 * [char * user] - User logging in. It could be null, for example, on startup
 */
void spawn_shell(int forkno, char * user){
	int shellpid;
	if(!(shellpid = fork())) {
		/* Redirect IO */
		grab_io(fd_slave);

		/* Call shell/login session */
		if(forkno > 0) {
			if(user == NULL) {
				char * tokens[] = {"/bin/login", "-q", NULL};
				execv(tokens[0], tokens);
			} else {
				char * tokens[] = {"/bin/login", "-q", "-u", user, NULL};
				execv(tokens[0], tokens);
			}
		} else {
			char * tokens[] = {"/bin/login", "-u", "root", NULL};
			execv(tokens[0], tokens);
		}
	} else {
		char forkno_str[10];
		sprintf(forkno_str, "%d", forkno);
		hashmap_set(shellpid_hash, forkno_str, (void*)shellpid);
		if(user)
			hashmap_set(multishell_sessions, user, (void*)shellpid);
	}
}

int get_pid_from_hash(int shellno) {
	char shellno_str[5];
	sprintf(shellno_str, "%d", shellno);
	return (int)hashmap_get(shellpid_hash, shellno_str);
}

char * get_username_from_pid(int pid) {
	char * username = NULL;
	list_t * keys = hashmap_keys(multishell_sessions);
	foreach(key,keys)
		if((int)hashmap_get(multishell_sessions, key->value) == pid) {
			int usr_size = strlen(key->value);
			username = malloc(usr_size);
			memset(username, 0, usr_size);
			memcpy(username, key->value, usr_size);
			username[usr_size] = '\0';
			break;
		}
	list_freeall(keys);
	return username;
}

int get_shellno_count() {
	return hashmap_size(shellpid_hash);
}

void * parent_killshells(void * garbage) {
	while(!exit_application) {
		/* Kill the children! Kill them all! */
		int shellcount = get_shellno_count();
		for(int i=0;i < shellcount;i++) {
			int child = get_pid_from_hash(i);

			if(waitpid(child, NULL, WNOHANG) > 0){ /* One of them died */
				char forkno_str[10];
				sprintf(forkno_str,"%d", i);
				hashmap_remove(shellpid_hash, forkno_str);
				char * user = get_username_from_pid(child);
				hashmap_remove(multishell_sessions, user);
				free(user);

				/* Update the other shellno's onward: */
				for(int j = i;j < shellcount - 1;j++){
					char forkno_next_str[10];
					sprintf(forkno_str, "%d", j);
					sprintf(forkno_next_str, "%d", (j+1));
					hashmap_set(shellpid_hash, forkno_str, hashmap_get(shellpid_hash,forkno_next_str));
				}
				/* And finally, delete the last one */
				char forkno_last[10];
				sprintf(forkno_last, "%d", shellcount-1);
				hashmap_remove(shellpid_hash, forkno_last);

				shells_forked--;
			}
		}
	}

	/* Really shouldn't reach this point */
	return NULL;
	pthread_exit(0);
}

static char * respond_to_client(int pid) {
	char * shmon_response = malloc(SHM_MAX_ID_SIZE);
	sprintf(shmon_response, "%d", pid);
	return shmon_response;
}

char * shmon_callback(shm_packet_t * packet) {
	shmon_packet_t * shmon_packet = (shmon_packet_t*)packet->dat;
	switch(shmon_packet->cmd) {
	case SHMON_CTRL_GRAB_PID: {
		/* Allocate shell and return its pid */
		/* Read the user from shared memory */
		char * user = shmon_packet->username;
		/* See if he is already logged in (through the hashmap) */
		if(hashmap_contains(multishell_sessions, user)) {
			/* return the user's pid instead of the new one (if he's logged in) */
			return respond_to_client((int)hashmap_get(multishell_sessions, user));
		} else {
			/* the user is not logged in, spawn new shell */
			spawn_shell(shells_forked, user);
			return respond_to_client(get_pid_from_hash(shells_forked++));
		}
		break;
	}
	case SHMON_CTRL_ADD_USR: {
		/* Add a user to the hashmap */
		/* Read the user from shared memory */
		char * user = shmon_packet->username;
		int pid = atoi(shmon_packet->cmd_dat);
		hashmap_set(multishell_sessions, user, (void*)pid);
		break;
	}
	case SHMON_CTRL_GET_USR: {
		/* Get a user from the hashmap */
		/* Grab process pid from user IF he exists */
		/* Read the user from shared memory */
		char * user = shmon_packet->username;
		if(hashmap_contains(multishell_sessions, user))
			return respond_to_client((int)hashmap_get(multishell_sessions, user));
		else
			return respond_to_client(-1);
		break;
	}
	case SHMON_CTRL_REDIR: {
		/* Sends packet to the destination process, causing that process to 'wake up'. */
		char packet_dest[SHM_MAX_ID_SIZE];
		sprintf(packet_dest, SHMON_CLIENT_LOGIN_FORMAT, atoi(shmon_packet->cmd_dat));
		char shm_msg[16] = SHMON_MSG_PROC_WAKE;

		shm_packet_t * pack = create_packet(shmon_server, packet_dest, SHM_DEV_CLIENT, shm_msg, strlen(shm_msg));
		shman_send_to_network_clear(shmon_server, pack, 0);

		free(pack);
		break;
	}
	}

	char * ret = malloc(SHM_MAX_PACKET_DAT_SIZE);
	ret[0] = 1; /* just tells it is finished really. The client won't be expecting data to return, so it's not important what number we put here */
	return ret;
}

void init_shell_monitor() {
	shellpid_hash = (hashmap_t*)hashmap_create(MAX_MULTISHELL);
	multishell_sessions = (hashmap_t*)hashmap_create(MAX_MULTISHELL);
	shmon_server = shman_create_server(SHMON_SERVER_IP);
}

void monitor_multishell() {
	if(!fork()) {
		init_shell_monitor();

		/* Create one shell at startup */
		for(int i = 0;i < MULTISHELL_STARTUP_COUNT;i++) {
			spawn_shell(i, NULL);
			shells_forked++;
		}

		/* Monitor multishell requests and reap child processes: */
		pthread_t parent_killchild;
		pthread_create(&parent_killchild, NULL, parent_killshells, NULL);

		shman_server_listen(shmon_server, shmon_callback);

		/* Better not reach this point */

		hashmap_free(multishell_sessions);
		hashmap_free(shellpid_hash);
		free(multishell_sessions);
		free(shellpid_hash);
	}
}
