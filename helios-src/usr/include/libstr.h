/*
 * libstr.h
 *
 *  Created on: Aug 28, 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_LIBSTR_H_
#define HELIOS_SRC_USR_INCLUDE_LIBSTR_H_

typedef struct {
	char ** str;
	int wordcount;
} split_t;

extern split_t split(char * str, char deli);
void free_split(split_t str);
char *trim(char *str);

#endif /* HELIOS_SRC_USR_INCLUDE_LIBSTR_H_ */
