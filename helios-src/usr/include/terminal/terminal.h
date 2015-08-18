/*
 * terminal.h
 *
 *  Created on: Aug 17, 2015
 *      Author: miguel
 */

#ifndef HELIOS_SRC_USR_INCLUDE_TERMINAL_TERMINAL_H_
#define HELIOS_SRC_USR_INCLUDE_TERMINAL_TERMINAL_H_

#include <stdint.h>

#define TERM_BUFF_SIZE 256

/* Triggers escape mode. */
#define ANSI_ESCAPE  27
/* Escape verify */
#define ANSI_BRACKET '['
#define ANSI_BRACKET_RIGHT ']'
#define ANSI_OPEN_PAREN '('
/* Anything in this range (should) exit escape mode. */
#define ANSI_LOW    'A'
#define ANSI_HIGH   'z'
/* Escape commands */
#define ANSI_CUU    'A' /* Cursor Up                  */
#define ANSI_CUD    'B' /* Cursor Down                */
#define ANSI_CUF    'C' /* Cursor Forward             */
#define ANSI_CUB    'D' /* Cursor Back                */
#define ANSI_CNL    'E' /* Cursor Next Line           */
#define ANSI_CPL    'F' /* Cursor Previous Line       */
#define ANSI_CHA    'G' /* Cursor Horizontal Absolute */
#define ANSI_CUP    'H' /* Cursor Position            */
#define ANSI_ED     'J' /* Erase Data                 */
#define ANSI_EL     'K' /* Erase in Line              */
#define ANSI_SU     'S' /* Scroll Up                  */
#define ANSI_SD     'T' /* Scroll Down                */
#define ANSI_HVP    'f' /* Horizontal & Vertical Pos. */
#define ANSI_SGR    'm' /* Select Graphic Rendition   */
#define ANSI_DSR    'n' /* Device Status Report       */
#define ANSI_SCP    's' /* Save Cursor Position       */
#define ANSI_RCP    'u' /* Restore Cursor Position    */
#define ANSI_HIDE   'l' /* DECTCEM - Hide Cursor      */
#define ANSI_SHOW   'h' /* DECTCEM - Show Cursor      */
/* Display flags */
#define ANSI_BOLD      0x01
#define ANSI_UNDERLINE 0x02
#define ANSI_ITALIC    0x04
#define ANSI_ALTFONT   0x08 /* Character should use alternate font */
#define ANSI_SPECBG    0x10
#define ANSI_BORDER    0x20
#define ANSI_WIDE      0x40 /* Character is double width */
#define ANSI_CROSS     0x80 /* And that's all I'm going to support (for now) */

#define ANSI_EXT_IOCTL 'z'  /* These are special escapes only we support */

/* Default color settings */
#define TERM_DEFAULT_FG     0x07 /* Index of default foreground */
#define TERM_DEFAULT_BG     0x10 /* Index of default background */
#define TERM_DEFAULT_FLAGS  0x00 /* Default flags for a cell */
#define TERM_DEFAULT_OPAC   0xF2 /* For background, default transparency */

typedef struct {
	uint32_t c;
	uint32_t fg;
	uint32_t bg;
	uint32_t flag;
} term_cell_t;

typedef struct {
	void (*writer)(char);
	void (*set_color)(uint32_t, uint32_t);
	void (*set_cur)(int, int);
	int (*get_cur_x)(void);
	int (*get_cur_y)(void);
	void (*set_cell)(int, int, uint32_t);
	void (*cls)(int);
	void (*scroll)(int);
	void (*redraw_cursor)(void);
	void (*input_buffer)(char* );
	void (*set_font_size)(float);
	void (*set_title)(char* );
} term_callback_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t last_x;
	uint16_t last_y;
	uint32_t t_width;
	uint32_t t_height;
	uint32_t fg;
	uint32_t bg;
	uint32_t flags;
	uint32_t esc;
	uint32_t box;
	uint32_t bufsize;
	uint32_t buff[TERM_BUFF_SIZE];
	term_callback_t * calls;
	int volatile lock;
	uint8_t is_mouse;
} term_state_t;

extern term_state_t * ansi_init(term_state_t * t, int w, int y, term_callback_t * calls);
extern void ansi_put(term_state_t * t, char c);

#endif /* HELIOS_SRC_USR_INCLUDE_TERMINAL_TERMINAL_H_ */