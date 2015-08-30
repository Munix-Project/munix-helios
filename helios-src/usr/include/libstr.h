/*
 * libstr.h
 *
 *  Created on: Aug 28, 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_LIBSTR_H_
#define HELIOS_SRC_USR_INCLUDE_LIBSTR_H_

#include <stdint.h>

typedef struct {
	char ** str;
	int wordcount;
} split_t;

extern split_t split(char * str, char deli);
extern void free_split(split_t str);
extern char *trim(char *str);
extern char * str_replace(char *orig, char *rep, char *with);
extern uint8_t str_contains(char * str, char * needle);

#endif /* HELIOS_SRC_USR_INCLUDE_LIBSTR_H_ */
