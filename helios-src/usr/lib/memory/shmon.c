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

extern int fd_slave;
extern volatile int exit_application;

/* Shared memory "channels": */
char * shm_monitor_input;
char * shm_monitor_output;

static int shells_forked = 0;
static int shm_lock = 0;
hashmap_t * shellpid_hash;
hashmap_t * multishell_sessions;

void spawn_shell(int forkno, char * user){
	int shellpid;
	if(!(shellpid = fork())) {
		/* Redirect IO */
		for(int i=0;i < 3;i++)
			dup2(fd_slave, STDIN_FILENO + i);

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
		if(user!=NULL)
			hashmap_set(multishell_sessions, user, (void*)shellpid);
	}
}

void update_shm_shmon(int pid){
	char shmon_info[50];
	sprintf(shmon_info, "newshell:%d", pid);
	strcpy(shm_monitor_input, shmon_info);
}

int get_pid_from_hash(int shellno) {
	char shellno_str[5];
	sprintf(shellno_str,"%d", shellno);
	return (int)hashmap_get(shellpid_hash, shellno_str);
}

char * get_username_from_pid(int pid) {
	char * username = NULL;
	list_t * keys = hashmap_keys(multishell_sessions);
	foreach(key,keys)
		if((int)hashmap_get(multishell_sessions, key->value) == pid) {
			int usr_size = strlen(key->value);
			username = malloc(sizeof(char) * usr_size);
			memset(username, 0, usr_size);
			memcpy(username, key->value, usr_size);
			break;
		}
	list_freeall(keys);
	return username;
}

char * read_username_from_shm () {
	int usrname_size = 0;
	int usrname_offset = strchr(shm_monitor_output, ':') - shm_monitor_output;
	for(usrname_size=0; shm_monitor_output[usrname_size+usrname_offset]!='\n' ;usrname_size++);
	char * user = malloc(sizeof(char) * usrname_size);
	memcpy(user, strchr(shm_monitor_output, ':') + 1, usrname_size - 1);
	user[usrname_size - 1] = '\0';
	return user;
}

int get_shellno_count() {
	return hashmap_size(shellpid_hash);
}

void monitor_multishell() {
	if(!fork()) {
		shellpid_hash = (hashmap_t*)hashmap_create(MAX_MULTISHELL);
		multishell_sessions = (hashmap_t*)hashmap_create(MAX_MULTISHELL);

		/* Create two shells at startup */
		for(int i=0;i < MULTISHELL_STARTUP_COUNT;i++) {
			spawn_shell(i, NULL);
			shells_forked++;
		}

		/* Setup shared memory: */
		size_t shmon_s = 24;
		/* read only from other processes perspective */
		shm_monitor_input = (char *) syscall_shm_obtain(SHM_SHELLMON_IN, &shmon_s);
		/* write only from other processes perspective */
		shm_monitor_output = (char *) syscall_shm_obtain(SHM_SHELLMON_OUT, &shmon_s);
		memset(shm_monitor_output, 0, shmon_s);
		update_shm_shmon(get_pid_from_hash(shells_forked - 1));

		/* Monitor multishell requests and reap child processes: */
		while(!exit_application) {

			/* Kill the children! Kill them all! */
			int shellcount = get_shellno_count();
			for(int i=0;i < shellcount;i++){
				int child = get_pid_from_hash(i);

				if(waitpid(child, NULL, WNOHANG)>0){ /* One of them died */

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

			/* Use a lock/mutex on the shared memory resource */
			spin_lock(&shm_lock);
			/* Check if there's new requests: */
			if(shm_monitor_output[0]!=0) {
				/* Process request */
				if(shm_monitor_output[0] == '-') { /* We found a command for shmon */
					switch(shm_monitor_output[1]) {
					case SHM_CTRL_GRAB_PID: { /* Allocate shell and return its pid */
						/* Read the user from shared memory */
						char * user = read_username_from_shm();

						/* See if he is already logged (through the hashmap) */
						if(hashmap_contains(multishell_sessions, user)) {
							/* return the user's pid instead of the new one (if he's logged in) */
							update_shm_shmon((int)hashmap_get(multishell_sessions, user));
						} else {
							/* the user is not logged in, spawn new shell */
							spawn_shell(shells_forked, user);
							update_shm_shmon(get_pid_from_hash(shells_forked));
							shells_forked++;
						}
						free(user);
						break;
					}
					case SHM_CTRL_ADD_USR: { /* Add a user to the hashmap */
						/* Read the user from shared memory */
						char * user = read_username_from_shm();
						int pid = atoi((char*)strchr(shm_monitor_output, '\n') + 1);

						hashmap_set(multishell_sessions, user, (void*)pid);
						free(user);
						break;
					}
					case SHM_CTRL_GET_USR: { /* Get a user from the hashmap */
						/* Grab process pid from user IF he exists */
						/* Read the user from shared memory */
						char * user = read_username_from_shm();
						if(hashmap_contains(multishell_sessions, user))
							update_shm_shmon((int)hashmap_get(multishell_sessions, user));
						else
							update_shm_shmon(-1);
						free(user);
						break;
					}
					}
				}

				/* Clear request buffer */
				memset(shm_monitor_output, 0, shmon_s);
			}
			spin_unlock(&shm_lock);
		}

		/* Better not reach this point */

		hashmap_free(multishell_sessions);
		hashmap_free(shellpid_hash);
		free(multishell_sessions);
		free(shellpid_hash);
		syscall_shm_release(SHM_SHELLMON_IN);
		syscall_shm_release(SHM_SHELLMON_OUT);
	}
}
