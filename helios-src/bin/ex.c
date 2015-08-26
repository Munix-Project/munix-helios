/*
 * ex.c
 *
 *  Created on: Aug 26, 2015
 *      Author: miguel
 */

#include <syscall.h>
#include <stdio.h>

void request_shell(char * dat){
	size_t s = 0;
	char * shm = (char*)syscall_shm_obtain(SHM_SHELLMON_OUT, &s);
	strcpy(shm, dat);
}

void kill_shell(int shellno) {
	char itos[5];
	memset(itos, 0, 5);
	sprintf(itos, "-1:%d", shellno);
	request_shell(itos);
}

void read_shmon(){
	size_t s = 0;
	char * shm = (char*)syscall_shm_obtain(SHM_SHELLMON_IN, &s);
	printf("%s\n", shm); fflush(stdout);
}

int main(int argc, char ** argv) {
	if(argc == 2)
		request_shell("1");
	else if(argc == 3)
		kill_shell(atoi(argv[2]));
	else
		read_shmon();

	return 0;
}
