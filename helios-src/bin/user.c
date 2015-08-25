#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <pwd.h>
#include <termios.h>
#include <unistd.h>
#include <security/crypt/sha2.h>
#include <security/helios_auth.h>

uint8_t fl_usradd = 0;
uint8_t fl_usrdel = 0;
uint8_t fl_usradd_to_group = 0;
uint8_t fl_usrdel_from_group = 0;
uint8_t fl_groupadd = 0;
uint8_t fl_groupdel = 0;
uint8_t fl_emitpasswd = 0; /* TODO: Don't use this flag just yet */
uint8_t fl_isadmin = 0;

#define CHUNK_SIZE 4096

#define M_PASSWD "/etc/master.passwd"
#define PASSWD "/etc/passwd"
#define HOME "/home"

#define READLINE(c) (c = getc(passwd)) != '\n' && c != EOF

char * master_passwd[CHUNK_SIZE];
char * passwd[CHUNK_SIZE];

int parse_uid(char * userline) {
	struct group * g;
	char * ptr = NULL;
	int ocurr = 0, ptr_end = 0;
	uint8_t start_counting_end = 0;

	for(int i=0;i<strlen(userline);i++) {
		if(userline[i] == ':') ocurr++;

		if(start_counting_end)
			ptr_end++;

		if(ocurr==2) {
			ptr = userline + i + 2;
			start_counting_end = 1;
		}

		if(ocurr == 3) break;
	}

	char int_str[5];
	memcpy(int_str, ptr, ptr_end);
	int_str[ptr_end] = '\0';
	return atoi(int_str);
}

void rebuild_passwd(int linecount, int linetodelete, char * filepath) {
	FILE * passwd = fopen(filepath, "r+");

	/* Put all lines into a buffer */
	int passwd_len = linecount - 1;
	char ** passwd_buff = (char**) malloc(sizeof(char**) * passwd_len);
	char c = 0;
	int passwd_line = 0, i = 0, j = 0;
	while(c != EOF) {
		passwd_buff[i] = malloc(sizeof(char*) * CHUNK_SIZE);
		while(READLINE(c))
			if(i!=linetodelete)
				passwd_buff[passwd_line][j++] = c;
		if(i!=linetodelete)
			passwd_buff[passwd_line++][j] = '\0';
		j = 0;
		i++;
	}

	/* TODO: Order user's UID to prevent holes */

	/* Write everything back: */
	fclose(passwd);
	passwd = fopen(filepath, "w");
	for(int i=0;i < passwd_len;i++)
		fprintf(passwd, "%s\n", passwd_buff[i]);
	fclose(passwd);

	/* Cleanup: */
	for(int i=0;i < passwd_len;i++)
		free(passwd_buff[i]);
	free(passwd_buff);
}

static void err_msg(char * msg) {
	fprintf(stderr, "user: error: %s\n", msg);
	exit(EXIT_FAILURE);
}

uint8_t user_exists(char * username) {
	return getpwnam(username) > 0;
}

uint8_t group_exists(char * group) {
	/* TODO */
	return 0;
}

struct termios disable_echo() {
	struct termios old, new;
	tcgetattr(fileno(stdin), &old);
	new = old;
	new.c_lflag &= (~ECHO);
	tcsetattr(fileno(stdin), TCSAFLUSH, &new);
	return old;
}

void enable_echo(struct termios old) {
	tcsetattr(fileno(stdin), TCSAFLUSH, &old);
}

char * ask_passwd(uint8_t apply_sha2) {
	const int PASSLEN = 1024;
	char * passwd = malloc(sizeof(char) * PASSLEN);
	printf("%spassword: ", apply_sha2 ? "new " : "");
	fflush(stdout);
	struct termios old = disable_echo();

	fgets(passwd, PASSLEN, stdin);
	passwd[strlen(passwd)-1] = '\0';

	enable_echo(old);
	fprintf(stdout, "\n");

	/* Get hash from password: */
	if(apply_sha2) {
		char * hash = malloc(sizeof(char) * SHA512_DIGEST_STRING_LENGTH);
		SHA512_Data(passwd, strlen(passwd), hash);

		free(passwd);
		return hash;
	} else
		return passwd;
}

int getnewuid(struct passwd * p, FILE * masterpasswd) {
	int lastuid = -1;

	if(fl_isadmin) {
		/* UID for admin */
		while((p = (struct passwd*) fgetpwent(masterpasswd))) {
			if(p->pw_uid < 1000)
				lastuid = p->pw_uid;
		}
	} else {
		/* UID for user: */
		while((p = (struct passwd*) fgetpwent(masterpasswd)))
			if(p->pw_uid >= 1000)
				lastuid = p->pw_uid;
	}
	return lastuid + 1;
}

uint8_t are_we_admin() {
	return getuid() < 1000;
}

void adduser(char * username) {
	/* A user just tried to create an admin account: */
	if(!are_we_admin() && fl_isadmin)
		err_msg("only root or an administrator can add new administrator accounts");

	/* Check if user exists already: */
	if(user_exists(username)) {
		char buff[128];
		sprintf(buff, "%s user already exists", username);
		err_msg(buff);
	}

	char * pass = ask_passwd(1);

	/* Find UID: */
	FILE * masterpasswd = fopen(M_PASSWD,"r");
	struct passwd * p;
	int newuid = getnewuid(p, masterpasswd);
	fclose(masterpasswd);

	/* Set user data: */
	char home[256];
	sprintf(home, "%s/%s", HOME, username);

	char user_type[50];
	sprintf(user_type, fl_isadmin ? "Administrator" : "Local User");

	char passwd_buff[CHUNK_SIZE];
	sprintf(passwd_buff, "%s:x:%d:%d:%s:%s:/bin/sh:simple\n", username, newuid, newuid, user_type, home);

	char m_passwd_buff[CHUNK_SIZE];
	sprintf(m_passwd_buff, "%s:%s:%d:%d:%s:%s:/bin/sh:simple\n", username , pass, newuid, newuid, user_type, home);
	free(pass); /* We don't need the password hash anymore */

	/* Append to both files: */
	FILE * passwd = fopen(PASSWD, "a");
	fprintf(passwd, passwd_buff);
	fclose(passwd);
	masterpasswd = fopen(M_PASSWD, "a");
	fprintf(masterpasswd, m_passwd_buff);
	fclose(masterpasswd);

	/* And now create the home folder */
	char buff[256];
	sprintf(buff, "mkdir %s", home);
	system(buff);
}

uint8_t is_deleting_itself(char * username) {
	/* Check if the user is trying to delete itself while logged in */
	int thisuid = getuid();
	struct passwd * p;
	FILE * master_passwd = fopen(M_PASSWD, "r");
	while ((p = fgetpwent(master_passwd)))
		if(p->pw_uid == thisuid && !strcmp(p->pw_name, username))
			return 1;
	fclose(master_passwd);
	return 0;
}

void deluser(char * username) {
	/* Validate request for account deletion: */
	if(!strcmp(username,"root")) /* wow... why would you even?... */
		err_msg("cannot delete root!");

	if(is_deleting_itself(username))
		err_msg("cannot delete this account while logged in on it");

	if(!are_we_admin() && getpwnam(username)->pw_uid < 1000)
		err_msg("only root or an administrator can delete administrator accounts");

	if(!user_exists(username)) {
		char buff[128];
		sprintf(buff, "%s user does not exist", username);
		err_msg(buff);
	}

	/* All good, proceed to delete it */

	char * pass = NULL;
	while(1) {
		pass = ask_passwd(0);
		int uid = helios_auth(username, pass);
		if (uid < 0) {
			printf("\nLogin failed.\n"); fflush(stdout);
			free(pass);
			continue;
		}
		break;
	}
	free(pass);

	/* Ask for confirmation: */
	printf("Are you sure? (N/y): "); fflush(stdout);
	char ans = getc(stdin);
	if(ans != 'Y' && ans != 'y') return;

	/* delete nth line from passwd and master.passwd */
	struct passwd * p;
	FILE * master = fopen(M_PASSWD, "r");
	int user_line = -1, i = 0;
	while ((p = fgetpwent(master))) {
		if(!strcmp(p->pw_name, username))
			user_line = i;
		i++;
	}
	fclose(master);

	/* Remove the line and reorder the UID's at the same time */
	rebuild_passwd(i, user_line, PASSWD);
	rebuild_passwd(i, user_line, M_PASSWD);

	/* Finally, rename the home folder */
	char home[256];
	sprintf(home, "%s/%s", HOME, username);
	char buff[256];
	sprintf(buff, "rename %s %s_bak", home, username);
	system(buff);
}

void addusertogroup(char * username, char * group) {
	/* TODO */
	/* Check if user exists already: */
	printf("%s - %s\n", username, group); fflush(stdout);
}

void deluserfromgroup(char * username, char * group) {
	/* TODO */
	/* Check if user exists: */
	printf("%s - %s\n", username, group); fflush(stdout);
}

void addgroup(char * group) {
	/* TODO */
	/* Check if group exists already: */
	printf("%s\n", group); fflush(stdout);
}

void delgroup(char * group) {
	/* TODO */
	/* Check if user exists: */
	printf("%s\n", group); fflush(stdout);
}

void usage(FILE * f){
	fprintf(f, "user: usage: user [options]\n"
			"Options:\n"
			" --adduser [username] - adds a user\n"
			" --deluser [username] - removes a user\n"
			" --addusergroup [username] [groupname] - adds a user to a group\n"
			" --delusergroup [username] [groupname] - removes a user from a group\n"
			" --addgroup [groupname] - adds a group\n"
			" --delgroup [groupname] - removes a group\n"
			" -e - don't ask for password\n"
			" -a - add user as admin\n"
			" -h - shows this message\n");
}

int main(int argc, char ** argv) {
	if(argc<2) {
		usage(stderr);
		exit(EXIT_FAILURE);
	}

	char * username = NULL;
	char * username_group = NULL;
	char * group = NULL;

	/* Set flags: */
	while(1) {
		static struct option long_op[] = {
				{"adduser", 1, 0, 'u'},
				{"deluser", 1, 0, 'd'},
				{"addusergroup",1, 0, 's'},
				{"delusergroup",1, 0, 'l'},
				{"addgroup", 1,	0, 'g'},
				{"delgroup", 1,	0, 'r'}
		};

		int op_index;
		int c = getopt_long(argc, argv, "heaudslgr", long_op, &op_index);
		if(c==-1) break;

		switch(c) {
		case 'a': fl_isadmin = 1; break;
		case 'u': fl_usradd = 1; username = optarg; break;
		case 'd': fl_usrdel = 1; username = optarg; break;
		case 's':
			if(argc > 3) {
				if(argv[optind][0] != '-') {
					fl_usradd_to_group = 1;
					username_group = optarg;
					group = argv[optind];
				}
				else {
					err_msg("--addusergroup: Expected group name. Argument given");
				}
			} else {
				err_msg("--addusergroup: Expected group name.");
			}
			break;
		case 'l':
			if(argc > 3) {
				if(argv[optind][0] != '-') {
					fl_usrdel_from_group = 1;
					username_group = optarg;
					group = argv[optind];
				}
				else {
					err_msg("--delusergroup: Expected group name. Argument given");
				}
			} else {
				err_msg("--delusergroup: Expected group name.");
			}
			break;
		case 'g': fl_groupadd = 1; group = optarg; break;
		case 'r': fl_groupdel = 1; group = optarg; break;
		case 'e': fl_emitpasswd = 1; break;
		case 'h': usage(stdout); exit(EXIT_SUCCESS); break;
		}

	}

	/* Now do the tasks in the right order: */

	/* First check the flags: */
	if(fl_usradd && fl_usrdel)
		err_msg("Cannot add and remove a user at the same time.");
	if(fl_usradd_to_group && fl_usrdel_from_group)
		err_msg("Cannot add and remove a user to/from a group at the same time.");
	if(fl_groupadd && fl_groupdel)
		err_msg("Cannot add and remove a group at the same time.");

	/* Good, the input is clean, let's actually do the work: */
	//setbuffers(); /* used for parsing the user accounts and passwords */

	if(fl_usradd)
		adduser(username);
	if(fl_usrdel)
		deluser(username);
	if(fl_groupadd)
		addgroup(group);
	if(fl_groupdel)
		delgroup(group);
	if(fl_usradd_to_group)
		addusertogroup(username_group, group);
	if(fl_usrdel_from_group)
		deluserfromgroup(username_group, group);

	return 0;
}
