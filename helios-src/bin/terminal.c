/*
 * terminal.c
 *
 *  Created on: Aug 17, 2015
 *      Author: miguel
 */
#include <stdlib.h>
#include <stdio.h>
#include <terminal/terminal.h>

/************** SECTION 1: ANSI *********************/
#ifdef _KERNEL_
# include <system.h>
# include <types.h>
# include <logging.h>
static void _spin_lock(volatile int * foo) { return; }
static void _spin_unlock(volatile int * foo) { return; }
# define rgba(r,g,b,a) (((uint32_t)a * 0x1000000) + ((uint32_t)r * 0x10000) + ((uint32_t)g * 0x100) + ((uint32_t)b * 0x1))
# define rgb(r,g,b) rgba(r,g,b,0xFF)
#else
#include <stdlib.h>
#include <math.h>
#include <syscall.h>
#include <spinlock.h>
#include <graphics/graphics.h>
#include <graphics/vga-color.h>
#define _spin_lock spin_lock
#define _spin_unlock spin_unlock
#endif

#define MAX_ARGS 1024

static wchar_t box_chars[] = L"▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥";

/* Returns the lower of two shorts */
static uint16_t min(uint16_t a, uint16_t b) {
	return (a < b) ? a : b;
}

/* Returns the higher of two shorts */
static uint16_t max(uint16_t a, uint16_t b) {
	return (a > b) ? a : b;
}

/* Write the contents of the buffer, as they were all non-escaped data. */
static void ansi_dump_buffer(term_state_t * s) {
	for (int i = 0; i < s->bufsize; ++i)
		s->calls->writer(s->buff[i]);
}

/* Add to the internal buffer for the ANSI parser */
static void ansi_buf_add(term_state_t * s, char c) {
	if (s->bufsize >= TERM_BUFF_SIZE-1) return;
	s->buff[s->bufsize] = c;
	s->bufsize++;
	s->buff[s->bufsize] = '\0';
}

static int to_eight(uint32_t codepoint, char * out) {
	memset(out, 0x00, 7);

	if (codepoint < 0x0080) {
		out[0] = (char)codepoint;
	} else if (codepoint < 0x0800) {
		out[0] = 0xC0 | (codepoint >> 6);
		out[1] = 0x80 | (codepoint & 0x3F);
	} else if (codepoint < 0x10000) {
		out[0] = 0xE0 | (codepoint >> 12);
		out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[2] = 0x80 | (codepoint & 0x3F);
	} else if (codepoint < 0x200000) {
		out[0] = 0xF0 | (codepoint >> 18);
		out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[3] = 0x80 | ((codepoint) & 0x3F);
	} else if (codepoint < 0x4000000) {
		out[0] = 0xF8 | (codepoint >> 24);
		out[1] = 0x80 | (codepoint >> 18);
		out[2] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[3] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[4] = 0x80 | ((codepoint) & 0x3F);
	} else {
		out[0] = 0xF8 | (codepoint >> 30);
		out[1] = 0x80 | ((codepoint >> 24) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 18) & 0x3F);
		out[3] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[4] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[5] = 0x80 | ((codepoint) & 0x3F);
	}

	return strlen(out);
}

static void _ansi_put(term_state_t * s, char c) {
	term_callback_t * callbacks = s->calls;
	switch (s->esc) {
		case 0:
			/* We are not escaped, check for escape character */
			if (c == ANSI_ESCAPE) {
				/*
				 * Enable escape mode, setup a buffer,
				 * fill the buffer, get out of here.
				 */
				s->esc    = 1;
				s->bufsize    = 0;
				ansi_buf_add(s, c);
				return;
			} else if (c == 0) {
				return;
			} else {
				if (s->box && c >= 'a' && c <= 'z') {
					char buf[7];
					char *w = (char *)&buf;
					to_eight(box_chars[c-'a'], w);
					while (*w) {
						callbacks->writer(*w);
						w++;
					}
				} else {
					callbacks->writer(c);
				}
			}
			break;
		case 1:
			/* We're ready for [ */
			if (c == ANSI_BRACKET) {
				s->esc = 2;
				ansi_buf_add(s, c);
			} else if (c == ANSI_BRACKET_RIGHT) {
				s->esc = 3;
				ansi_buf_add(s, c);
			} else if (c == ANSI_OPEN_PAREN) {
				s->esc = 4;
				ansi_buf_add(s, c);
			} else {
				/* This isn't a bracket, we're not actually escaped!
				 * Get out of here! */
				ansi_dump_buffer(s);
				callbacks->writer(c);
				s->esc = 0;
				s->bufsize = 0;
				return;
			}
			break;
		case 2:
			if (c >= ANSI_LOW && c <= ANSI_HIGH) {
				/* Woah, woah, let's see here. */
				char * pch;  /* tokenizer pointer */
				char * save; /* strtok_r pointer */
				char * argv[MAX_ARGS]; /* escape arguments */
				/* Get rid of the front of the buffer */
				strtok_r(s->buff,"[",&save);
				pch = strtok_r(NULL,";",&save);
				/* argc = Number of arguments, obviously */
				int argc = 0;
				while (pch != NULL) {
					argv[argc] = (char *)pch;
					++argc;
					if (argc > MAX_ARGS)
						break;
					pch = strtok_r(NULL,";",&save);
				}
				/* Alright, let's do this */
				switch (c) {
					case ANSI_EXT_IOCTL:
						{
							if (argc > 0) {
								int arg = atoi(argv[0]);
								switch (arg) {
									case 1:
										callbacks->redraw_cursor();
										break;
#ifndef _KERNEL_
									case 1555:
										if (argc > 1) {
											callbacks->set_font_size(atof(argv[1]));
										}
										break;
#endif
									default:
										break;
								}
							}
						}
						break;
					case ANSI_SCP:
						{
							s->last_x = callbacks->get_cur_x();
							s->last_y = callbacks->get_cur_y();
						}
						break;
					case ANSI_RCP:
						{
							callbacks->set_cur(s->last_x, s->last_y);
						}
						break;
					case ANSI_SGR:
						/* Set Graphics Rendition */
						if (argc == 0) {
							/* Default = 0 */
							argv[0] = "0";
							argc    = 1;
						}
						for (int i = 0; i < argc; ++i) {
							int arg = atoi(argv[i]);
							if (arg >= 100 && arg < 110) {
								/* Bright background */
								s->bg = 8 + (arg - 100);
								s->flags |= ANSI_SPECBG;
							} else if (arg >= 90 && arg < 100) {
								/* Bright foreground */
								s->fg = 8 + (arg - 90);
							} else if (arg >= 40 && arg < 49) {
								/* Set background */
								s->bg = arg - 40;
								s->flags |= ANSI_SPECBG;
							} else if (arg == 49) {
								s->bg = TERM_DEFAULT_BG;
								s->flags &= ~ANSI_SPECBG;
							} else if (arg >= 30 && arg < 39) {
								/* Set Foreground */
								s->fg = arg - 30;
							} else if (arg == 39) {
								/* Default Foreground */
								s->fg = 7;
							} else if (arg == 9) {
								/* X-OUT */
								s->flags |= ANSI_CROSS;
							} else if (arg == 7) {
								/* INVERT: Swap foreground / background */
								uint32_t temp = s->fg;
								s->fg = s->bg;
								s->bg = temp;
							} else if (arg == 6) {
								/* proprietary RGBA color support */
								if (i == 0) { break; }
								if (i < argc) {
									int r = atoi(argv[i+1]);
									int g = atoi(argv[i+2]);
									int b = atoi(argv[i+3]);
									int a = atoi(argv[i+4]);
									if (a == 0) a = 1; /* Override a = 0 */
									uint32_t c = rgba(r,g,b,a);
									if (atoi(argv[i-1]) == 48) {
										s->bg = c;
										s->flags |= ANSI_SPECBG;
									} else if (atoi(argv[i-1]) == 38) {
										s->fg = c;
									}
									i += 4;
								}
							} else if (arg == 5) {
								/* Supposed to be blink; instead, support X-term 256 colors */
								if (i == 0) { break; }
								if (i < argc) {
									if (atoi(argv[i-1]) == 48) {
										/* Background to i+1 */
										s->bg = atoi(argv[i+1]);
										s->flags |= ANSI_SPECBG;
									} else if (atoi(argv[i-1]) == 38) {
										/* Foreground to i+1 */
										s->fg = atoi(argv[i+1]);
									}
									++i;
								}
							} else if (arg == 4) {
								/* UNDERLINE */
								s->flags |= ANSI_UNDERLINE;
							} else if (arg == 3) {
								/* ITALIC: Oblique */
								s->flags |= ANSI_ITALIC;
							} else if (arg == 2) {
								/* Konsole RGB color support */
								if (i == 0) { break; }
								if (i < argc - 2) {
									int r = atoi(argv[i+1]);
									int g = atoi(argv[i+2]);
									int b = atoi(argv[i+3]);
									uint32_t c = rgb(r,g,b);
									if (atoi(argv[i-1]) == 48) {
										/* Background to i+1 */
										s->bg = c;
										s->flags |= ANSI_SPECBG;
									} else if (atoi(argv[i-1]) == 38) {
										/* Foreground to i+1 */
										s->fg = c;
									}
									i += 3;
								}
							} else if (arg == 1) {
								/* BOLD/BRIGHT: Brighten the output color */
								s->flags |= ANSI_BOLD;
							} else if (arg == 0) {
								/* Reset everything */
								s->fg = TERM_DEFAULT_FG;
								s->bg = TERM_DEFAULT_BG;
								s->flags = TERM_DEFAULT_FLAGS;
							}
						}
						break;
					case ANSI_SHOW:
						if (argc > 0) {
							if (!strcmp(argv[0], "?1049")) {
								callbacks->cls(2);
								callbacks->set_cur(0,0);
							} else if (!strcmp(argv[0], "?1000")) {
								s->is_mouse = 1;
							} else if (!strcmp(argv[0], "?1002")) {
								s->is_mouse = 2;
							}
						}
						break;
					case ANSI_HIDE:
						if (argc > 0) {
							if (!strcmp(argv[0], "?1049")) {
								/* TODO: Unimplemented */
							} else if (!strcmp(argv[0], "?1000")) {
								s->is_mouse = 0;
							} else if (!strcmp(argv[0], "?1002")) {
								s->is_mouse = 0;
							}
						}
						break;
					case ANSI_CUF:
						{
							int i = 1;
							if (argc) {
								i = atoi(argv[0]);
							}
							callbacks->set_cur(min(callbacks->get_cur_x() + i, s->t_width - 1), callbacks->get_cur_y());
						}
						break;
					case ANSI_CUU:
						{
							int i = 1;
							if (argc) {
								i = atoi(argv[0]);
							}
							callbacks->set_cur(callbacks->get_cur_x(), max(callbacks->get_cur_y() - i, 0));
						}
						break;
					case ANSI_CUD:
						{
							int i = 1;
							if (argc) {
								i = atoi(argv[0]);
							}
							callbacks->set_cur(callbacks->get_cur_x(), min(callbacks->get_cur_y() + i, s->t_height - 1));
						}
						break;
					case ANSI_CUB:
						{
							int i = 1;
							if (argc) {
								i = atoi(argv[0]);
							}
							callbacks->set_cur(max(callbacks->get_cur_x() - i,0), callbacks->get_cur_y());
						}
						break;
					case ANSI_CHA:
						if (argc < 1) {
							callbacks->set_cur(0,callbacks->get_cur_y());
						} else {
							callbacks->set_cur(min(max(atoi(argv[0]), 1), s->t_width) - 1, callbacks->get_cur_y());
						}
						break;
					case ANSI_CUP:
						if (argc < 2) {
							callbacks->set_cur(0,0);
						} else {
							callbacks->set_cur(min(max(atoi(argv[1]), 1), s->t_width) - 1, min(max(atoi(argv[0]), 1), s->t_height) - 1);
						}
						break;
					case ANSI_ED:
						if (argc < 1)
							callbacks->cls(0);
						else
							callbacks->cls(atoi(argv[0]));
						break;
					case ANSI_EL:
						{
							int what = 0, x = 0, y = 0;
							if (argc >= 1) {
								what = atoi(argv[0]);
							}
							if (what == 0) {
								x = callbacks->get_cur_x();
								y = s->t_width;
							} else if (what == 1) {
								x = 0;
								y = callbacks->get_cur_x();
							} else if (what == 2) {
								x = 0;
								y = s->t_width;
							}
							for (int i = x; i < y; ++i) {
								callbacks->set_cell(i, callbacks->get_cur_y(), ' ');
							}
						}
						break;
					case ANSI_DSR:
						{
							char out[24];
							sprintf(out, "\033[%d;%dR", callbacks->get_cur_y() + 1, callbacks->get_cur_x() + 1);
							callbacks->input_buffer(out);
						}
						break;
					case ANSI_SU:
						{
							int how_many = 1;
							if (argc > 0) {
								how_many = atoi(argv[0]);
							}
							callbacks->scroll(how_many);
						}
						break;
					case ANSI_SD:
						{
							int how_many = 1;
							if (argc > 0) {
								how_many = atoi(argv[0]);
							}
							callbacks->scroll(-how_many);
						}
						break;
					case 'X':
						{
							int how_many = 1;
							if (argc > 0) {
								how_many = atoi(argv[0]);
							}
							for (int i = 0; i < how_many; ++i) {
								callbacks->writer(' ');
							}
						}
						break;
					case 'd':
						if (argc < 1)
							callbacks->set_cur(callbacks->get_cur_x(), 0);
						else
							callbacks->set_cur(callbacks->get_cur_x(), atoi(argv[0]) - 1);
						break;
					default:
						/* Meh */
						break;
				}
				/* Set the states */
				if (s->flags & ANSI_BOLD && s->fg < 9) {
					callbacks->set_color(s->fg % 8 + 8, s->bg);
				} else {
					callbacks->set_color(s->fg, s->bg);
				}
				/* Clear out the buffer */
				s->bufsize = 0;
				s->esc = 0;
				return;
			} else {
				/* Still escaped */
				ansi_buf_add(s, c);
			}
			break;
		case 3:
			if (c == '\007') {
				/* Tokenize on semicolons, like we always do */
				char * pch;  /* tokenizer pointer */
				char * save; /* strtok_r pointer */
				char * argv[MAX_ARGS]; /* escape arguments */
				/* Get rid of the front of the buffer */
				strtok_r(s->buff,"]",&save);
				pch = strtok_r(NULL,";",&save);
				/* argc = Number of arguments, obviously */
				int argc = 0;
				while (pch != NULL) {
					argv[argc] = (char *)pch;
					++argc;
					if (argc > MAX_ARGS) break;
					pch = strtok_r(NULL,";",&save);
				}
				/* Start testing the first argument for what command to use */
				if (argv[0]) {
					if (!strcmp(argv[0], "1")) {
						if (argc > 1) {
							callbacks->set_title(argv[1]);
						}
					} /* Currently, no other options */
				}
				/* Clear out the buffer */
				s->bufsize = 0;
				s->esc = 0;
				return;
			} else {
				/* Still escaped */
				if (c == '\n' || s->bufsize == 255) {
					ansi_dump_buffer(s);
					callbacks->writer(c);
					s->bufsize = 0;
					s->esc = 0;
					return;
				}
				ansi_buf_add(s, c);
			}
			break;
		case 4:
			if (c == '0') {
				s->box = 1;
			} else if (c == 'B') {
				s->box = 0;
			} else {
				ansi_dump_buffer(s);
				callbacks->writer(c);
			}
			s->esc = 0;
			s->bufsize = 0;
			break;
	}
}

void ansi_put(term_state_t * s, char c) {
	_spin_lock(&s->lock);
	_ansi_put(s, c);
	_spin_unlock(&s->lock);
}

term_state_t * ansi_init(term_state_t * s, int w, int y, term_callback_t * callbacks_in) {
	if (!s) {
		s = malloc(sizeof(term_state_t));
	}

	memset(s, 0, sizeof(term_state_t));

	/* Terminal Defaults */
	s->fg     = TERM_DEFAULT_FG;    /* Light grey */
	s->bg     = TERM_DEFAULT_BG;    /* Black */
	s->flags  = TERM_DEFAULT_FLAGS; /* Nothing fancy*/
	s->t_width  = w;
	s->t_height = y;
	s->box    = 0;
	s->calls = callbacks_in;
	s->calls->set_color(s->fg, s->bg);
	s->is_mouse = 0;

	return s;
}

/************** SECTION 2: TERMINAL "DRIVER" FOR ANSI *********************/
#include <pthread_os.h>
#include <errno.h>
#include <utf8decode.h>
#include <unistd.h>
#include <fcntl.h>
#include <kbd.h>

#define TERM_NAME "helios"
#define CHAR_WIDTH 1
#define CHAR_HEIGHT 1

int fd_master, fd_slave;
FILE* terminal;
term_cell_t * term_buffer = NULL;
term_state_t * ansi_state = NULL;

uint16_t t_width = 80,
		t_height = 25,
		cursor_y = 0,
		cursor_x = 0;

uint32_t curr_fg = 7,
		curr_bg = 0;

uint8_t is_cursor = 1,
		is_logging = 0;

volatile int exit_app = 0;

/* ANSI-to-VGA */
char vga_to_ansi[] = {
	0, 4, 2, 6, 1, 5, 3, 7,
	8,12,10,14, 9,13,11,15
};

wchar_t box_chars_in[] = L"▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥▄";
wchar_t box_chars_out[] =  {176,0,0,0,0,248,241,0,0,217,191,218,192,197,196,196,196,196,196,195,180,193,194,179,243,242,220};

unsigned short * textmemptr = (unsigned short *)0xB8000;

static uint32_t vga_base_colors[] = {
	0x000000,
	0xAA0000,
	0x00AA00,
	0xAA5500,
	0x0000AA,
	0xAA00AA,
	0x00AAAA,
	0xAAAAAA,
	0x555555,
	0xFF5555,
	0x55AA55,
	0xFFFF55,
	0x5555FF,
	0xFF55FF,
	0x55FFFF,
	0xFFFFFF,
};

uint32_t child_pid = 0;

static unsigned int timer_tick = 0;

uint32_t scrollback_offset = 0;
uint8_t hold_out = 0; /* state indicator on last cell ignore \n */

#define INPUT_SIZE 1024
char input_buffer[INPUT_SIZE];
int  input_collected = 0;

static int is_gray(uint32_t a) {
	int a_r = (a & 0xFF0000) >> 16;
	int a_g = (a & 0xFF00) >> 8;
	int a_b = (a & 0xFF);

	return (a_r == a_g && a_g == a_b);
}

void placech(unsigned char c, int x, int y, int attr) {
	unsigned short *where;
	unsigned att = attr << 8;
	where = textmemptr + (y * 80 + x);
	*where = c | att;
}

static int color_distance(uint32_t a, uint32_t b) {
	int a_r = (a & 0xFF0000) >> 16;
	int a_g = (a & 0xFF00) >> 8;
	int a_b = (a & 0xFF);

	int b_r = (b & 0xFF0000) >> 16;
	int b_g = (b & 0xFF00) >> 8;
	int b_b = (b & 0xFF);

	int distance = 0;
	distance += abs(a_r - b_r) * 3;
	distance += abs(a_g - b_g) * 6;
	distance += abs(a_b - b_b) * 10;

	return distance;
}

uint32_t ununicode(uint32_t c) {
	wchar_t * w = box_chars_in;
	while (*w) {
		if (*w == c) {
			return box_chars_out[w-box_chars_in];
		}
		w++;
	}
	switch (c) {
		case L'»': return 175;
		case L'·': return 250;
	}
	return 4;
}

static int best_match(uint32_t a) {
	int best_distance = INT32_MAX;
	int best_index = 0;
	for (int j = 0; j < 16; ++j) {
		if (is_gray(a) && !is_gray(vga_base_colors[j]));
		int distance = color_distance(a, vga_base_colors[j]);
		if (distance < best_distance) {
			best_index = j;
			best_distance = distance;
		}
	}
	return best_index;
}

void term_write_char(uint32_t c, uint16_t x, uint16_t y, uint32_t fg, uint32_t bg, uint8_t flags) {
	if (c > 128) c = ununicode(c);
	if (fg > 256) {
		fg = best_match(fg);
	}
	if (bg > 256) {
		bg = best_match(bg);
	}
	if (fg > 16) {
		fg = vga_colors[fg];
	}
	if (bg > 16) {
		bg = vga_colors[bg];
	}
	if (fg == 16) fg = 0;
	if (bg == 16) bg = 0;
	placech(c, x, y, (vga_to_ansi[fg] & 0xF) | (vga_to_ansi[bg] << 4));
}

static void cell_redraw_inverted(uint16_t x, uint16_t y) {
	if (x >= t_width || y >= t_height) return;

	/* Fetch cell: */
	term_cell_t * cell = (term_cell_t *)((uintptr_t)term_buffer + (y * t_width + x) * sizeof(term_cell_t));

	/* Write to it: */
	if (((uint32_t *)cell)[0] == 0x00000000)
		term_write_char(' ', x * CHAR_WIDTH, y * CHAR_HEIGHT, TERM_DEFAULT_BG, TERM_DEFAULT_FG, TERM_DEFAULT_FLAGS | ANSI_SPECBG);
	 else
		term_write_char(cell->c, x *  CHAR_WIDTH, y * CHAR_HEIGHT, cell->bg, cell->fg, cell->flag | ANSI_SPECBG);
}

static void cell_redraw(uint16_t x, uint16_t y) {
	if(x >= t_width || y>= t_height) return;

	/* Fetch cell: */
	term_cell_t * c = (term_cell_t*)((uintptr_t) term_buffer + (y * t_width + x) * sizeof(term_cell_t));

	/* Write to it: */
	if (((uint32_t *)c)[0] == 0x00000000)
		term_write_char(' ', x * CHAR_WIDTH, y * CHAR_HEIGHT, TERM_DEFAULT_FG, TERM_DEFAULT_BG, TERM_DEFAULT_FLAGS);
	else
		term_write_char(c->c, x * CHAR_WIDTH, y * CHAR_HEIGHT, c->fg, c->bg, c->flag);
}

void term_redraw_all(void) {
	for(uint16_t y = 0; y < t_height; y++)
		for(uint16_t x = 0; x < t_width; x++)
			cell_redraw(x , y);
}

void * wait_for_exit(void * garbage) {
	int pid;
	do
		pid = waitpid(-1, NULL, 0);
	while (pid == -1 && errno == EINTR);

	exit_app = 1;

	char exit_msg[] = "[Process terminated\n]";
	write(fd_slave, exit_msg, sizeof(exit_msg));
}

void clear_input() {
	memset(input_buffer, 0, INPUT_SIZE);
	input_collected = 0;
}

void handle_input(char c) {
	write(fd_master, &c, 1);
}

void handle_input_s(char * c) {
	write(fd_master, c, strlen(c));
}

void key_evt(int ret, key_event_t * evt) {
	if(ret) {
		if(evt->modifiers & KEY_MOD_LEFT_ALT || evt->modifiers & KEY_MOD_RIGHT_ALT)
			handle_input('\033');

		handle_input(evt->key);
	} else {
		if(evt->action == KEY_ACTION_UP) return;
		switch(evt->keycode) {
		case KEY_F1: handle_input_s("\033OP"); break;
		case KEY_F2: handle_input_s("\033OQ"); break;
		case KEY_F3: handle_input_s("\033OR"); break;
		case KEY_F4: handle_input_s("\033OS"); break;
		case KEY_F5: handle_input_s("\033[15~"); break;
		case KEY_F6: handle_input_s("\033[17~"); break;
		case KEY_F7: handle_input_s("\033[18~"); break;
		case KEY_F8: handle_input_s("\033[19~"); break;
		case KEY_F9: handle_input_s("\033[20~"); break;
		case KEY_F10: handle_input_s("\033[21~"); break;
		case KEY_F11: handle_input_s("\033[23~"); break;
		case KEY_F12: handle_input_s("\033[24~"); break;
		case KEY_ARROW_UP: handle_input_s("\033[A"); break;
		case KEY_ARROW_DOWN: handle_input_s("\033[B"); break;
		case KEY_ARROW_RIGHT : handle_input_s("\033[C"); break;
		case KEY_ARROW_LEFT: handle_input_s("\033[D");break;
		case KEY_PAGE_UP: handle_input_s("\033[5~"); break;
		case KEY_PAGE_DOWN: handle_input_s("\033[6~"); break;
		}
	}
}

void * handle_kbd(void * garbage) {
	int kbd = open("/dev/kbd", O_RDONLY);
	key_event_t evt;
	char c;
	key_event_state_t kbd_state = {0};

	struct stat s;
	fstat(kbd, &s);
	for(int i=0;i < s.st_size;i++) {
		char tmp[1];
		read(kbd, tmp, 1);
	}

	/* Now listen to the keyboard: */
	while(!exit_app) {
		int k = read(kbd, &c, 1);
		if(k > 1) {
			int ret = kbd_scancode(&kbd_state, c, &evt);
			key_evt(ret, &evt);
		}
	}
	pthread_exit(0);
}

void render_cursor() {
	cell_redraw_inverted(cursor_x, cursor_y);
}

void flip_cursor() {
	static uint8_t cursor_flipped = 0;
	if(scrollback_offset != 0) return;

	if(cursor_flipped)
		cell_redraw(cursor_x, cursor_y);
	else
		render_cursor();

	cursor_flipped = 1 - cursor_flipped;
}

void * blink_cursor(void * garbage) {
	while(!exit_app) {
		timer_tick++;
		if(timer_tick == 5) {
			timer_tick = 0;
			flip_cursor();
		}
		usleep(90000);
	}
	pthread_exit(0);
}

void draw_cursor() {
	if(!is_cursor) return;

	timer_tick = 0;
	render_cursor();
}

int is_wide(uint32_t codepoint) {
	if(codepoint < 256) return 0;
	return wcwidth(codepoint) == 2;
}

/* Callbacks: */
void term_write(char c) {
	static uint32_t codepoint = 0;
	static uint32_t unicode_state = 0;

	cell_redraw(cursor_x, cursor_y);
	if (!decode(&unicode_state, &codepoint, (uint8_t)c)) {
		if (c == '\r') {
			cursor_x = 0;
			return;
		}
		if (cursor_x == t_width) {
			cursor_x = 0;
			++cursor_y;
		}
		if (cursor_y == t_height) {
			term_scroll(1);
			cursor_y = t_height - 1;
		}
		if (c == '\n') {
			if (cursor_x == 0 && hold_out) {
				hold_out = 0;
				return;
			}
			++cursor_y;
			if (cursor_y == t_height) {
				term_scroll(1);
				cursor_y = t_height - 1;
			}
			draw_cursor();
		} else if (c == '\007') {
			/* bell */
#if USE_BELL
			for (int i = 0; i < t_height; ++i) {
				for (int j = 0; j < t_width; ++j) {
					cell_redraw_inverted(j, i);
				}
			}
			syscall_nanosleep(0,10);
			term_redraw_all();
#endif
		} else if (c == '\b') {
			if (cursor_x > 0) {
				--cursor_x;
			}
			cell_redraw(cursor_x, cursor_y);
			draw_cursor();
		} else if (c == '\t') {
			cursor_x += (8 - cursor_x % 8);
			draw_cursor();
		} else {
			int wide = is_wide(codepoint);
			uint8_t flags = ansi_state->flags;
			if (wide && cursor_x == t_width - 1) {
				cursor_x = 0;
				++cursor_y;
			}
			if (wide) {
				flags = flags | ANSI_WIDE;
			}
			cell_set(cursor_x,cursor_y, codepoint, curr_fg, curr_bg, flags);
			cell_redraw(cursor_x,cursor_y);
			cursor_x++;
			if (wide && cursor_x != t_width) {
				cell_set(cursor_x, cursor_y, 0xFFFF, curr_fg, curr_bg, ansi_state->flags);
				cell_redraw(cursor_x,cursor_y);
				cell_redraw(cursor_x-1,cursor_y);
				cursor_x++;
			}
		}
	} else if (unicode_state == UTF8_REJECT) {
		unicode_state = 0;
	}
	draw_cursor();
}

void term_set_colors(uint32_t fg, uint32_t bg) {
	curr_fg = fg;
	curr_bg = bg;
}

void term_set_cursor(int x, int y) {
	cell_redraw(cursor_x, cursor_y);
}

int term_get_cursor_x() {
	return cursor_x;
}

int term_get_cursor_y() {
	return cursor_y;
}

void cell_set(uint16_t x, uint16_t y, uint32_t c, uint32_t fg, uint32_t bg, uint8_t flags) {
	if (x >= t_width || y >= t_height) return;

	term_cell_t * cell = (term_cell_t *)((uintptr_t)term_buffer + (y * t_width + x) * sizeof(term_cell_t));
	cell->c     = c;
	cell->fg    = fg;
	cell->bg    = bg;
	cell->flag = flags;
}

void term_set_cell(int x, int y, int c) {
	cell_set(x, y, c, curr_fg, curr_bg, ansi_state->flags);
	cell_redraw(x, y);
}

void term_clear(int i) {
	if(i==2){
		cursor_x = cursor_y = 0;
		memset((void*)term_buffer, 0, t_width * t_height * sizeof(term_cell_t));
		term_redraw_all();
	} else if (i == 0) {
		for(int x = cursor_x; x < t_width; x++)
			term_set_cell(x, cursor_y, ' ');
		for(int y = cursor_y; y < t_height; y++)
			for(int x = 0; x < t_width; x++)
				term_set_cell(x, y , ' ');
	} else if (i == 1) {
		for (int y = 0; y < cursor_y; ++y)
			for (int x = 0; x < t_width; ++x)
				term_set_cell(x, y, ' ');

		for (int x = 0; x < cursor_x; ++x)
			term_set_cell(x, cursor_y, ' ');
	}
}

void term_scroll(int quant) {
	if (quant >= t_height || -quant >= t_height) {
		term_clear(0); /*xxx decide which int to pass on here*/
		return;
	}
	if (quant == 0) {
		return;
	}
	if (quant > 0) {
		/* Shift terminal cells one row up */
		memmove(term_buffer, (void *)((uintptr_t)term_buffer + sizeof(term_cell_t) * t_width), sizeof(term_cell_t) * t_width * (t_height - quant));
		/* Reset the "new" row to clean cells */
		memset((void *)((uintptr_t)term_buffer + sizeof(term_cell_t) * t_width * (t_height - quant)), 0x0, sizeof(term_cell_t) * t_width * quant);
		for (int i = 0; i < quant; ++i) {
			for (uint16_t x = 0; x < t_width; ++x) {
				cell_set(x,t_height - quant,' ', curr_fg, curr_bg, ansi_state->flags);
			}
		}
		term_redraw_all();
	} else {
		quant = -quant;
		/* Shift terminal cells one row up */
		memmove((void *)((uintptr_t)term_buffer + sizeof(term_cell_t) * t_width), term_buffer, sizeof(term_cell_t) * t_width * (t_height - quant));
		/* Reset the "new" row to clean cells */
		memset(term_buffer, 0x0, sizeof(term_cell_t) * t_width * quant);
		term_redraw_all();
	}
}

void term_redraw_cursor() {
	if(term_buffer) draw_cursor();
}

void input_buffer_stuff(char * str) {
	write(fd_master, str, strlen(str) + 1);
}

static void set_term_font_size(float s) {
	/* xxx not yet implemented */
}

void set_title(char * c) {
	/* xxx do nothing, atleast for now */
}

term_callback_t t_callbacks = {
		&term_write,
		&term_set_colors,
		&term_set_cursor,
		&term_get_cursor_x,
		&term_get_cursor_y,
		&term_set_cell,
		&term_clear,
		&term_scroll,
		&term_redraw_cursor,
		&input_buffer_stuff,
		&set_term_font_size,
		&set_title,
};
/* End callbacks */

void reinit(int signal) {
	// TODO: Hide cursor with asm: __asm__ __volatile__("outb %1, %0");

	if(!term_buffer){
		term_buffer = malloc(t_width * t_height * sizeof(term_cell_t));
		memset(term_buffer, 0, t_width * t_height * sizeof(term_cell_t));
	}

	ansi_state = ansi_init(ansi_state, t_width, t_height, &t_callbacks);
	term_redraw_all();
}

int main(int argc, char * argv[]) {
	char buff[32];
	sprintf(buff, "TERM=%s", TERM_NAME);
	putenv(buff);

	/* Login by default (FOR NOW!) xxx */
	is_logging = 1;

	syscall_openpty(&fd_master, &fd_slave, NULL, NULL, NULL);
	terminal = fdopen(fd_slave, "w");

	reinit(0);
	fflush(stdin);

	int thispid = getpid();
	uint32_t newproc = fork();

	if(thispid != getpid()) {
		/* Split the two processes */
		dup2(fd_slave, 0);
		dup2(fd_slave, 1);
		dup2(fd_slave, 2);

		if(is_logging) {
			/* And launch login! */
			char * toks [] = {"/bin/login", NULL};
			execvp(toks[0], toks);
		} else {
			/* Not logging in, launch shell instead */
			char * sh = getenv("SHELL");
			if(!sh) sh ="/bin/sh";
			char * toks [] = {sh, NULL};
			execvp(toks[0], toks);
		}
		exit_app = 1;
		return 1;
	} else { /* The other process */
		child_pid = thispid;

		/* Create threads to be used on the terminal: */
		pthread_t wait_for_exit_th;
		pthread_create(&wait_for_exit_th, NULL, wait_for_exit, NULL);

		pthread_t handle_kbd_th;
		pthread_create(&handle_kbd_th, NULL, handle_kbd, NULL);

		pthread_t cursor_blink_th;
		pthread_create(&cursor_blink_th, NULL, blink_cursor, NULL);

		/* Actually output something through ansi: */
		unsigned char buff[1024];
		while(!exit_app) {
			int r = read(fd_master, buff, 1024);
			for(uint32_t i=0; i < r;i++)
				ansi_put(ansi_state, buff[i]);
		}
	}

	return 0;
}
