/*
 * file_it.h
 *
 *  Created on: Aug 28, 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_FILE_IT_H_
#define HELIOS_SRC_USR_INCLUDE_FILE_IT_H_

#include <stdint.h>

/* File Reader */
#define ALL_LINES -1
#define IGNORE_DEPTH -1
typedef void (*file_callback_t)(char*);

extern void file_read_all(file_callback_t call, char * filepath, int nthline);

/* Directory crawler */

enum FIT_TYPES {
	FIT_TYPE_DIR 	= 'd',
	FIT_TYPE_EXEC 	= 'e',
	FIT_TYPE_LINK 	= 'l',
	FIT_TYPE_FILE 	= 'f',
	FIT_TYPE_BLOCK 	= 'b'
};

typedef struct {
	char * fullpath;
	char * name;
	uint8_t type;
	int size;
} fit_dir_cback_dat;

typedef void (*fit_dir_cback_t)(fit_dir_cback_dat);

extern void dir_crawl(fit_dir_cback_t call, char * path, int depth, int maxdepth);

#endif /* HELIOS_SRC_USR_INCLUDE_FILE_IT_H_ */
