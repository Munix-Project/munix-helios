/*
 * sh.c
 *
 *  Created on: Aug 16, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <getopt.h>
#include <termios.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/wait.h>
#include <sys/utsname.h>

#include <kbd.h>

#define MAX_SHELL_CMDS 512

int shell_pid;
int child_pid = 0;
char usr_logged[1024];
char hostname[256];

uint32_t shell_commands_len = 0;
/* A shell command is like a C program */
typedef uint32_t(*shell_command_t) (int argc, char ** argv);
char * shell_commands[MAX_SHELL_CMDS];
shell_command_t shell_pointers[MAX_SHELL_CMDS]; /* Command functions */

int shell_scroll = 0;
char   shell_temp[1024];

int    shell_interactive = 1;
int    shell_force_raw   = 0;

/******* SECTION 1: EMBEDDED COMMANDS INTO THE SHELL *******/
#define GOTO_DIR(dir) if(chdir(dir)) goto cd_error;

#define SHELL_HISTORY_ENTRIES 128
char * shell_history[SHELL_HISTORY_ENTRIES];
int shell_history_count  = 0;
int shell_history_offset = 0;

#define CMPSTR(str1,str2) !strcmp(str1,str2)

uint32_t cmd_cd(int argc, char * argv[]) {
	if(argc > 1) {
		GOTO_DIR(argv[1]);
	} else {
		/*just using cd with no args*/
		char home = getenv("HOME");
		if(home) {
			GOTO_DIR(home);
		} else{
			char home_path[512];
			sprintf(home_path, "/home/%s", usr_logged);
			GOTO_DIR(home_path);
		}
	}
	return 0;
	cd_error:
	fprintf(stderr, "%s: could not cd '%s': no such file or directory\n", argv[0], argv[1]);
	return 1;
}

char * shell_history_get(int item);
uint32_t cmd_history(int argc, char * argv[]) {
	for (int i = 0; i < shell_history_count; ++i)
		printf("%d\t%s\n", i + 1, shell_history_get(i));
	return 0;
}

uint32_t cmd_export(int argc, char * argv[]) {
	if (argc > 1) putenv(argv[1]);
	return 0;
}

uint32_t cmd_exit(int argc, char * argv[]) {
	if (argc > 1) exit(atoi(argv[1]));
	else exit(0);
	return -1;
}

uint32_t cmd_set(int argc, char * argv[]) {
	char * term = getenv("TERM");
	if(!term || strstr(term, "helios" != term)) {
		fprintf(stderr, "Unrecognized terminal! The variable 'TERM' is not set/is invalid/not supported\n");
		return 1;
	}

	if(argc < 2) {
		fprintf(stderr, "%s: expected arguments\n", argv[0]);
		return 1;
	}

	/* Builtin sets: */
	if(CMPSTR(argv[1], "alpha")) {
		if (argc < 3) {
			fprintf(stderr, "%s %s [0 or 1]\n", argv[0], argv[1]);
			return 1;
		}

		if (atoi(argv[2])) printf("\033[2001z");
		else printf("\033[2000z");
		fflush(stdout);

		return 0;
	} else if(CMPSTR(argv[1], "size")) {
		if (argc < 4) {
			fprintf(stderr, "%s %s [width] [height]\n", argv[0], argv[1]);
			return 1;
		}
		printf("\033[3000;%s;%sz", argv[2], argv[3]);
		fflush(stdout);
	} else if(CMPSTR(argv[1], "force-raw")) {
		shell_force_raw = 1;
		return 0;
	} else if(CMPSTR(argv[1], "no-force-raw")) {
		shell_force_raw = 0;
		return 0;
	} else if(CMPSTR(argv[1], "--help")) {
		fprintf(stderr, "Available arguments:\n"
		                "  alpha - alpha transparency enabled / disabled\n"
		                "  scale - font scaling\n"
		                "  size - terminal width/height in characters\n"
		                "  force-raw - sets terminal to raw mode before commands\n"
		                "  no-force-raw - disables forced raw mode\n"
		);
		return 0;
	}

	fprintf(stderr, "%s: unrecognized argument\n", argv[0]);
	return 1;
}

/******* SECTION 2: THE SHELL ITSELF *******/

void shell_install_cmd(char * name, shell_command_t cmd) {
	if (shell_commands_len == MAX_SHELL_CMDS) {
		fprintf(stderr, "Ran out of space for static shell commands. The maximum number of commands is %d\n", MAX_SHELL_CMDS);
		return;
	}
	shell_commands[shell_commands_len] = name;
	shell_pointers[shell_commands_len] = cmd;
	shell_commands_len++;
}

void install_cmds(){
	shell_install_cmd("cd", cmd_cd);
	shell_install_cmd("history", cmd_history);
	shell_install_cmd("export", cmd_export);
	shell_install_cmd("exit", cmd_exit);
	shell_install_cmd("set", cmd_set);
}

char * shell_history_get(int item) {
	return shell_history[(item + shell_history_offset) % SHELL_HISTORY_ENTRIES];
}

void sig_pass(int sig) {
	if(child_pid)
		kill(child_pid, sig);
}

void get_user() {
	char * tmp = getenv("USER");
	if(tmp)
		strcpy(usr_logged, tmp);
	else
		sprintf(usr_logged, "%d", getuid());
}

void get_hostname() {
	struct utsname uts_buff;
	uname(&uts_buff);
	memcpy(hostname, uts_buff.nodename, strlen(uts_buff.nodename));
}

void fetch_os_executables() {
	DIR * dirbin = opendir("/bin");
	struct dirent * ent = readdir(dirbin);
	while(ent != NULL) {
		if(ent->d_name[0] != '.') {
			char * s = malloc(sizeof(char) * (strlen(ent->d_name) + 1));
			memcpy(s, ent->d_name, strlen(ent->d_name) + 1);
			shell_install_cmd(s, NULL);
		}

		ent = readdir(dirbin); /* Get next node in /bin */
	}
	/* All done */
	closedir(dirbin);
}

struct command {
	char * string;
	void * func;
};

static int comp_shell_commands(const void *p1, const void *p2) {
	return strcmp(((struct command *)p1)->string, ((struct command *)p2)->string);
}

void sort_commands() {
	struct command commands[MAX_SHELL_CMDS];
	for (int i = 0; i < shell_commands_len; ++i) {
		commands[i].string = shell_commands[i];
		commands[i].func   = shell_pointers[i];
	}
	qsort(&commands, shell_commands_len, sizeof(struct command), comp_shell_commands);
	for (int i = 0; i < shell_commands_len; ++i) {
		shell_commands[i] = commands[i].string;
		shell_pointers[i] = commands[i].func;
	}
}

int main(int argc, char * argv[]) {
	int no_wait = 0;
	int free_cmd = 0;
	int last_ret = 0;

	shell_pid = getpid();

	signal(SIGINT, sig_pass);
	signal(SIGWINCH, sig_pass);

	get_user();
	get_hostname();

	/* Install "pre-done" commands */
	install_cmds();

	/* Grab executable names from /bin */
	fetch_os_executables();

	sort_commands();

	/* Shell initialized. Execute the rest: */
	/* TODO */

	return 0;
}
