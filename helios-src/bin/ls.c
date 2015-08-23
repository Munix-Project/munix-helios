/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
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

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define MIN_COL_SPACING 2

/* flags: */
uint8_t fl_human_readable = 0;
uint8_t fl_mode_long = 0;
uint8_t fl_stdout_is_tty = 1;
uint8_t fl_show_hidden = 0;
uint8_t fl_echo_dir = 0;

int this_year = 0;

int term_width = 0;
int term_height = 0;

struct file_t {
	char * fname;
	struct stat statbuf;
	char * link;
	struct stat statbufl;
};

static const char * color_str(struct stat * sb);
static void update_col_widths(int * widths, struct file_t * file);

/* Sorting functions: */
static int filecmp(const void * c1, const void * c2) {
	const struct file_t * d1 = *(const struct file_t **)c1;
	const struct file_t * d2 = *(const struct file_t **)c2;

	int a = S_ISDIR(d1->statbuf.st_mode);
	int b = S_ISDIR(d2->statbuf.st_mode);

	if (a == b) return strcmp(d1->fname, d2->fname);
	else if (a < b) return -1;
	else if (a > b) return 1;
	return 0;
}

static int filecmp_notypesort(const void * c1, const void * c2) {
	const struct file_t * d1 = *(const struct file_t **)c1;
	const struct file_t * d2 = *(const struct file_t **)c2;
	return strcmp(d1->fname, d2->fname);
}

/* Display functions: */
static int print_usr(char * _out, int uid) {
	struct passwd * p = getpwuid(uid);
	int out = 0;
	if (p)
		out = sprintf(_out, "%s", p->pw_name);
	else
		out = sprintf(_out, "%d", uid);

	endpwent();
	return out;
}

static int print_human_readable_size(char * _out, size_t s) {
	if (s >= 1<<20) {
		size_t t = s / (1 << 20);
		return sprintf(_out, "%d.%1dM", (int)t, (int)(s - t * (1 << 20)) / ((1 << 20) / 10));
	} else if (s >= 1<<10) {
		size_t t = s / (1 << 10);
		return sprintf(_out, "%d.%1dK", (int)t, (int)(s - t * (1 << 10)) / ((1 << 10) / 10));
	} else {
		return sprintf(_out, "%d", (int)s);
	}
	return 0;
}

static void print_entry(struct file_t * file, int colwidth) {
	const char * ansi_color_str = color_str(&file->statbuf);

	/* Print the file name */
	if (fl_stdout_is_tty)
		printf("\033[%sm%s\033[0m", ansi_color_str, file->fname);
	else
		printf("%s", file->fname);

	/* Pad the rest of the column */
	for (int rem = colwidth - strlen(file->fname); rem > 0; rem--)
		printf(" ");
}

static void print_entry_long(int * widths, struct file_t * file) {
	const char * ansi_color_str = color_str(&file->statbuf);

	/* file permissions */
	if (S_ISLNK(file->statbuf.st_mode))       { printf("l"); }
	else if (S_ISCHR(file->statbuf.st_mode))  { printf("c"); }
	else if (S_ISBLK(file->statbuf.st_mode))  { printf("b"); }
	else if (S_ISDIR(file->statbuf.st_mode))  { printf("d"); }
	else { printf("-"); }
	printf( (file->statbuf.st_mode & S_IRUSR) ? "r" : "-");
	printf( (file->statbuf.st_mode & S_IWUSR) ? "w" : "-");
	if (file->statbuf.st_mode & S_ISUID)
		printf("s");
	else
		printf( (file->statbuf.st_mode & S_IXUSR) ? "x" : "-");
	printf( (file->statbuf.st_mode & S_IRGRP) ? "r" : "-");
	printf( (file->statbuf.st_mode & S_IWGRP) ? "w" : "-");
	printf( (file->statbuf.st_mode & S_IXGRP) ? "x" : "-");
	printf( (file->statbuf.st_mode & S_IROTH) ? "r" : "-");
	printf( (file->statbuf.st_mode & S_IWOTH) ? "w" : "-");
	printf( (file->statbuf.st_mode & S_IXOTH) ? "x" : "-");

	printf( " %*d ", widths[0], file->statbuf.st_nlink); /* number of links, not supported */

	char tmp[100];
	print_usr(tmp, file->statbuf.st_uid);
	printf("%-*s ", widths[1], tmp);
	print_usr(tmp, file->statbuf.st_gid);
	printf("%-*s ", widths[2], tmp);

	if (fl_human_readable) {
		print_human_readable_size(tmp, file->statbuf.st_size);
		printf("%*s ", widths[3], tmp);
	} else {
		printf("%*d ", widths[3], (int)file->statbuf.st_size);
	}

	char time_buf[80];
	struct tm * timeinfo = localtime(&file->statbuf.st_mtime);
	if (timeinfo->tm_year == this_year) {
		strftime(time_buf, 80, "%b %d %H:%M", timeinfo);
	} else {
		strftime(time_buf, 80, "%b %d  %Y", timeinfo);
	}
	printf("%s ", time_buf);

	/* Print the file name */
	if (fl_stdout_is_tty) {
		printf("\033[%sm%s\033[0m", ansi_color_str, file->fname);
		if (S_ISLNK(file->statbuf.st_mode)) {
			const char * s = color_str(&file->statbufl);
			printf(" -> \033[%sm%s\033[0m", s, file->link);
		}
	} else {
		printf("%s", file->fname);
		if (S_ISLNK(file->statbuf.st_mode))
			printf(" -> %s", file->link);
	}

	printf("\n");
}

static void display_files(struct file_t ** file_list, int entry_count) {
	if(fl_mode_long) {
		/* Display the list in a long mode */
		int widths[4] = {0,0,0,0};
		for(int i = 0;i < entry_count; i++)
			update_col_widths(widths, file_list[i]);
		for(int i = 0;i < entry_count; i++)
			print_entry_long(widths, file_list[i]);
		/* Done */
	} else {
		/* Simple / standard mode */

		/* Calculate the spaces and columns: */
		int entry_max_len = 0;
		for(int i = 0;i < entry_count; i++)
			entry_max_len = MAX(entry_max_len, strlen(file_list[i]->fname));

		int col_ext = entry_max_len + MIN_COL_SPACING;
		int cols = ((term_width - entry_max_len) / col_ext) + 1;

		/* Now print the entries: */
		for(int i = 0; i < entry_count; i++) {
			/* Columns */
			print_entry(file_list[i++], entry_max_len);

			for (int j = 0; (i < entry_count) && (j < (cols-1)); j++) {
				printf("  ");
				print_entry(file_list[i++], entry_max_len);
			}
			printf("\n");
		}
	}
}

static int display_dir(char * path) {
	DIR * dir = opendir(path);
	if(dir == NULL) return 2;

	if(fl_echo_dir) printf("%s:\n", path);

	/*
	 * This list variable will store the entries found
	 * on the directory later for display
	 */
	list_t * entries = list_create();

	struct dirent * dire = readdir(dir);
	while(dire != NULL) {

		/* Grab node (if not hidden or not forcing it to show hidden): */
		if(fl_show_hidden || (dire->d_name[0] != '.')) {
			struct file_t * f = malloc(sizeof(struct file_t));
			f->fname = (char*) strdup(dire->d_name);

			char tmp[strlen(path) + strlen(dire->d_name) + 1];
			sprintf(tmp, "%s/%s", path, dire->d_name);

			lstat(tmp, &f->statbuf);
			if(S_ISLNK(f->statbuf.st_mode)) {
				stat(tmp, &f->statbufl);
				f->link = malloc(4096);
				readlink(tmp, f->link, 4096);
			}

			list_insert(entries, (void *) f);
		}

		/* Get next entry in 'dir': */
		dire = readdir(dir);
	}
	closedir(dir);

	/* All entries grabbed! Time to display them */

	/* Sort them: */
	struct file_t ** file_arr = malloc(sizeof(struct tile_t *) * entries->length);
	int i = 0;
	foreach(node, entries) {
		file_arr[i++] = (struct file_t *) node->value;
	}

	list_free(entries);

	qsort(file_arr, i, sizeof(struct file_t *), filecmp_notypesort);

	display_files(file_arr, i);

	free(file_arr);
	return 0;
}

/* Output formatting functions: */
static const char * color_str(struct stat * sb) {
	if (S_ISDIR(sb->st_mode)) {
		/* Directory */
		return DIR_COLOR;
	} else if (S_ISLNK(sb->st_mode)) {
		/* Symbolic Link */
		return SYMLINK_COLOR;
	} else if (sb->st_mode & S_ISUID) {
		/* setuid - sudo, etc. */
		return SETUID_COLOR;
	} else if (sb->st_mode & 0111) {
		/* Executable */
		return EXE_COLOR;
	} else if (S_ISBLK(sb->st_mode) || S_ISCHR(sb->st_mode)) {
		/* Device file */
		return DEVICE_COLOR;
	} else {
		/* Regular file */
		return REG_COLOR;
	}
}

static void update_col_widths(int * widths, struct file_t * file) {
	char tmp[256];
	int n;

	/* Links */
	n = sprintf(tmp, "%d", file->statbuf.st_nlink);
	if (n > widths[0]) widths[0] = n;

	/* User */
	n = print_usr(tmp, file->statbuf.st_uid);
	if (n > widths[1]) widths[1] = n;

	/* Group */
	n = print_usr(tmp, file->statbuf.st_gid);
	if (n > widths[2]) widths[2] = n;

	/* File size */
	if (fl_human_readable)
		n = print_human_readable_size(tmp, file->statbuf.st_size);
	else
		n = sprintf(tmp, "%d", (int)file->statbuf.st_size);

	if (n > widths[3]) widths[3] = n;
}

int main(int argc, char ** argv) {
	/* Parse arguments */
	char * p = ".";

	if (argc > 1) {
		int c;
		while ((c = getopt(argc, argv, "ahl")) != -1) {
			switch (c) {
				case 'a':
					fl_show_hidden = 1;
					break;
				case 'h':
					fl_human_readable = 1;
					break;
				case 'l':
					fl_mode_long = 1;
					break;
			}
		}

		if (optind < argc)
			p = argv[optind];

		if(!strcmp(p, "~")) /* Check if argument is home */
			strcpy(p, getenv("HOME"));

		if (optind + 1 < argc)
			fl_echo_dir = 1;
	}

	fl_stdout_is_tty = isatty(STDOUT_FILENO);

	if (fl_mode_long) {
		struct tm * timeinfo;
		struct timeval now;
		gettimeofday(&now, NULL);
		timeinfo = localtime((time_t *)&now.tv_sec);
		this_year = timeinfo->tm_year;
	}

	if (fl_stdout_is_tty) {
		struct winsize w;
		ioctl(1, TIOCGWINSZ, &w);
		term_width = w.ws_col;
		term_height = w.ws_row;
		term_width -= 1; /* And this just helps clean up our math */
	}

	int out = 0;

	if(argc == 1 || optind == argc)
		display_dir(p); /* Current directory */
	else {
		list_t * files = list_create();
		while(p) {
			struct file_t * f = malloc(sizeof(struct file_t));
			f->fname = p;

			/* Check if file exists with stat: */
			int t = stat(p, &f->statbuf);
			if(t < 0) {
				printf("%s: cannot access %s: No such file or directory\n", argv[0], p);
				free(f);
				out = 2;
			} else {
				list_insert(files, f);
			}

			optind++;
			if(optind >= argc) p = NULL;
			else p = argv[optind];
		}

		/* Files fetched  */
		struct file_t ** file_arr = malloc(sizeof(struct file_t *) * files->length);
		int index = 0;
		foreach(node, files) {
			file_arr[index++] = (struct file_t *)node->value;
		}

		list_free(files);

		qsort(file_arr, index, sizeof(struct file_t *), filecmp);

		int first_dir = index;
		for(int i = 0; i < index;i++)
			if(S_ISDIR(file_arr[i]->statbuf.st_mode)){
				first_dir = i;
				break;
			}
		if(first_dir)
			display_files(file_arr, first_dir);

		for (int i = first_dir; i < index; ++i) {
			if (i != 0)
				printf("\n");
			display_dir(file_arr[i]->fname);
		}
	}
	return out;
}
