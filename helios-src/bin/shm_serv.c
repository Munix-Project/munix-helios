/*
 * shm_serv.c
 *
 *  Created on: 25 Sep 2015
 *      Author: miguel
 */

#include <shm.h>
#include <stdio.h>

int main(int argc, char ** argv) {
	shm_send_cmd(SHM_CMD_NEW_SERVER);
	return 0;
}
