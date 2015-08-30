/*
 * libstr.c
 *
 *  Created on: Aug 28, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libstr.h>

char *trim(char *str) {
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if(!str) return NULL;
    if(str[0] == '\0') return str;

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while(isspace(*frontp)) ++frontp;

    if(endp != frontp)
   		while(isspace(*(--endp)) && endp != frontp);

    if(str + len - 1 != endp)
    	*(endp + 1) = '\0';
    else
    	if(frontp != str && endp == frontp)
        	*str = '\0';

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if(frontp != str) {
    	while(*frontp) *endp++ = *frontp++;
    	*endp = '\0';
    }
    return str;
}

split_t split(char * str, char deli) {
	split_t ret = {
		.str = NULL,
		.wordcount = 0
	};

	/* Prepare string */
	str = trim(str);

	/* Count words: */
	int strl = strlen(str);
	int deli_count = 0;
	for(int i=0;i < strl;i++)
		if(str[i] == deli) {
			deli_count++;
			while(str[i + 1] == deli) i++; /* Avoid holes on the splitting */
		}
	if(!deli_count) {
		ret.str = malloc(sizeof(char **));
		ret.str[0] = malloc(sizeof(char*));
		strcpy(ret.str[0], str);
		ret.wordcount = 1;
		return ret; /* No delimiters were found */
	}

	/* Perform split: */
	int word_count = deli_count + 1;
	int off = 0, end;

	ret.wordcount = word_count;
	ret.str = malloc(sizeof(char **) * word_count);

	for(int i = 0;i < word_count;i++) {
		char * ptr = strchr(str + off, deli);
		if(ptr)
			end = ptr - (str + off);
		else
			end = strchr(str + off, 0) - (str + off);

		ret.str[i] = malloc(sizeof(char*) * (end + 1));
		memcpy(ret.str[i], str + off, end);
		ret.str[i][end] = '\0';

		off += end;
		while(str[off] == deli) off++; /* Avoid holes */
	}
	return ret;
}

void free_split(split_t str) {
	for(int i=0;i<str.wordcount;i++)
		free(str.str[i]);
	free(str.str);
}

/* str_replace source: http://stackoverflow.com/a/779960 */
// You must free the result if result is non-NULL.
char * str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count)
        ins = tmp + len_rep;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

uint8_t str_contains(char * str, char * needle){
	return strstr(str, needle) != NULL;
}
