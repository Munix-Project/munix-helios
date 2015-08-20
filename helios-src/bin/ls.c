/*
 * ls.c
 *
 *  Created on: Aug 20, 2015
 *      Author: miguel
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <pwd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <list.h>

/* ls' colors: */
#define A_B "\e["
#define B_A "m"
#define EXE_COLOR		"1;32"
#define DIR_COLOR		"1;34"
#define SYMLINK_COLOR	"1;36"
#define REG_COLOR		"0"
#define MEDIA_COLOR		""
#define SYM_COLOR		""
#define BROKEN_COLOR	"1;"
#define DEVICE_COLOR	"1;33;40"
#define SETUID_COLOR	"37;41"

struct file_t {
	char * fname;
	struct stat statbuf;
	char * link;
	struct stat statbufl;
};

static int display_dir(char * path) {
	DIR * dir = opendir(path);
	if(dir == NULL) return 2;

	list_t * entries = list_create();

	struct dirent * dire = readdir(dir);
	while(dire != NULL) {
		struct file_t * f = malloc(sizeof(struct file_t));
		f->fname; // todo
	}

	list_free(entries);

	return 0;
}

int main(int argc, char ** argv) {


	return 0;
}
