/*
 * rline.h
 *
 *  Created on: Aug 19, 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_RLINE_H_
#define HELIOS_SRC_USR_INCLUDE_RLINE_H_

typedef struct {
	char *  buffer;
	struct rline_callback * callbacks;
	int     collected;
	int     requested;
	int     newline;
	int     cancel;
	int     offset;
	int     tabbed;
} rline_context_t;

typedef void (*rline_callback_t)(rline_context_t * context);

typedef struct rline_callback {
	rline_callback_t tab_complete;
	rline_callback_t redraw_prompt;
	rline_callback_t special_key;
	rline_callback_t key_up;
	rline_callback_t key_down;
	rline_callback_t key_left;
	rline_callback_t key_right;
	rline_callback_t rev_search;
} rline_callbacks_t;

void rline_redraw(rline_context_t * context);
void rline_redraw_clean(rline_context_t * context);
int rline(char * buffer, int buf_size, rline_callbacks_t * callbacks);

#endif /* HELIOS_SRC_USR_INCLUDE_RLINE_H_ */
