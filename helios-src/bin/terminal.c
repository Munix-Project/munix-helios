/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
 */

#include <terminal/terminal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include <math.h>
#include <syscall.h>
#include <spinlock.h>
#include <graphics/graphics.h>
#define _spin_lock spin_lock
#define _spin_unlock spin_unlock
#endif

#define MAX_ARGS 1024

#define TERM_ENV "TERM=xterm"

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
	for (int i = 0; i < s->buflen; ++i) {
		s->callbacks->writer(s->buffer[i]);
	}
}

/* Add to the internal buffer for the ANSI parser */
static void ansi_buf_add(term_state_t * s, char c) {
	if (s->buflen >= TERM_BUF_LEN-1) return;
	s->buffer[s->buflen] = c;
	s->buflen++;
	s->buffer[s->buflen] = '\0';
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
	term_callbacks_t * callbacks = s->callbacks;
	switch (s->escape) {
		case 0:
			/* We are not escaped, check for escape character */
			if (c == ANSI_ESCAPE) {
				/*
				 * Enable escape mode, setup a buffer,
				 * fill the buffer, get out of here.
				 */
				s->escape    = 1;
				s->buflen    = 0;
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
				s->escape = 2;
				ansi_buf_add(s, c);
			} else if (c == ANSI_BRACKET_RIGHT) {
				s->escape = 3;
				ansi_buf_add(s, c);
			} else if (c == ANSI_OPEN_PAREN) {
				s->escape = 4;
				ansi_buf_add(s, c);
			} else {
				/* This isn't a bracket, we're not actually escaped!
				 * Get out of here! */
				ansi_dump_buffer(s);
				callbacks->writer(c);
				s->escape = 0;
				s->buflen = 0;
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
				strtok_r(s->buffer,"[",&save);
				pch = (char*) strtok_r(NULL,";",&save);
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
							s->save_x = callbacks->get_csr_x();
							s->save_y = callbacks->get_csr_y();
						}
						break;
					case ANSI_RCP:
						{
							callbacks->set_csr(s->save_x, s->save_y);
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
								callbacks->set_csr(0,0);
							} else if (!strcmp(argv[0], "?1000")) {
								s->mouse_on = 1;
							} else if (!strcmp(argv[0], "?1002")) {
								s->mouse_on = 2;
							}
						}
						break;
					case ANSI_HIDE:
						if (argc > 0) {
							if (!strcmp(argv[0], "?1049")) {
								/* TODO: Unimplemented */
							} else if (!strcmp(argv[0], "?1000")) {
								s->mouse_on = 0;
							} else if (!strcmp(argv[0], "?1002")) {
								s->mouse_on = 0;
							}
						}
						break;
					case ANSI_CUF:
						{
							int i = 1;
							if (argc)
								i = atoi(argv[0]);
							callbacks->set_csr(min(callbacks->get_csr_x() + i, s->width - 1), callbacks->get_csr_y());
						}
						break;
					case ANSI_CUU:
						{
							int i = 1;
							if (argc)
								i = atoi(argv[0]);
							callbacks->set_csr(callbacks->get_csr_x(), max(callbacks->get_csr_y() - i, 0));
						}
						break;
					case ANSI_CUD:
						{
							int i = 1;
							if (argc)
								i = atoi(argv[0]);
							callbacks->set_csr(callbacks->get_csr_x(), min(callbacks->get_csr_y() + i, s->height - 1));
						}
						break;
					case ANSI_CUB:
						{
							int i = 1;
							if (argc)
								i = atoi(argv[0]);
							callbacks->set_csr(max(callbacks->get_csr_x() - i,0), callbacks->get_csr_y());
						}
						break;
					case ANSI_CHA:
						if (argc < 1)
							callbacks->set_csr(0,callbacks->get_csr_y());
						else
							callbacks->set_csr(min(max(atoi(argv[0]), 1), s->width) - 1, callbacks->get_csr_y());
						break;
					case ANSI_CUP:
						if (argc < 2)
							callbacks->set_csr(0,0);
						else
							callbacks->set_csr(min(max(atoi(argv[1]), 1), s->width) - 1, min(max(atoi(argv[0]), 1), s->height) - 1);
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
								x = callbacks->get_csr_x();
								y = s->width;
							} else if (what == 1) {
								x = 0;
								y = callbacks->get_csr_x();
							} else if (what == 2) {
								x = 0;
								y = s->width;
							}
							for (int i = x; i < y; ++i)
								callbacks->set_cell(i, callbacks->get_csr_y(), ' ');
						}
						break;
					case ANSI_DSR:
						{
							char out[24];
							sprintf(out, "\033[%d;%dR", callbacks->get_csr_y() + 1, callbacks->get_csr_x() + 1);
							callbacks->input_buffer_stuff(out);
						}
						break;
					case ANSI_SU:
						{
							int how_many = 1;
							if (argc > 0)
								how_many = atoi(argv[0]);
							callbacks->scroll(how_many);
						}
						break;
					case ANSI_SD:
						{
							int how_many = 1;
							if (argc > 0)
								how_many = atoi(argv[0]);
							callbacks->scroll(-how_many);
						}
						break;
					case 'X':
						{
							int how_many = 1;
							if (argc > 0)
								how_many = atoi(argv[0]);
							for (int i = 0; i < how_many; ++i)
								callbacks->writer(' ');
						}
						break;
					case 'd':
						if (argc < 1)
							callbacks->set_csr(callbacks->get_csr_x(), 0);
						else
							callbacks->set_csr(callbacks->get_csr_x(), atoi(argv[0]) - 1);
						break;
					default:
						/* Meh */
						break;
				}
				/* Set the states */
				if ((s->flags & ANSI_BOLD) && (s->fg < 9))
					callbacks->set_color(s->fg % 8 + 8, s->bg);
				else
					callbacks->set_color(s->fg, s->bg);
				/* Clear out the buffer */
				s->buflen = 0;
				s->escape = 0;
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
				strtok_r(s->buffer,"]",&save);
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
				s->buflen = 0;
				s->escape = 0;
				return;
			} else {
				/* Still escaped */
				if (c == '\n' || s->buflen == 255) {
					ansi_dump_buffer(s);
					callbacks->writer(c);
					s->buflen = 0;
					s->escape = 0;
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
			s->escape = 0;
			s->buflen = 0;
			break;
	}
}

void ansi_put(term_state_t * s, char c) {
	_spin_lock(&s->lock);
	_ansi_put(s, c);
	_spin_unlock(&s->lock);
}

term_state_t * ansi_init(term_state_t * s, int w, int y, term_callbacks_t * callbacks_in) {

	if (!s)
		s = malloc(sizeof(term_state_t));

	memset(s, 0x00, sizeof(term_state_t));

	/* Terminal Defaults */
	s->fg     = TERM_DEFAULT_FG;    /* Light grey */
	s->bg     = TERM_DEFAULT_BG;    /* Black */
	s->flags  = TERM_DEFAULT_FLAGS; /* Nothing fancy*/
	s->width  = w;
	s->height = y;
	s->box    = 0;
	s->callbacks = callbacks_in;
	s->callbacks->set_color(s->fg, s->bg);
	s->mouse_on = 0;

	return s;
}
/************** SECTION 2: TERMINAL "DRIVER" FOR ANSI *********************/
#include <syscall.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <getopt.h>
#include <errno.h>
#include <wchar.h>
#include <utf8decode.h>
#include <pthread_os.h>
#include <kbd.h>
#include <graphics/vga-color.h>
#include <syscall.h>
#include <list.h>
#include <hashmap.h>

#define USE_BELL 0

/* Multi shell variables and constants */
#define MULTISHELL_STARTUP_COUNT 1
static int shells_forked = 0;
int shm_lock = 0;
hashmap_t * shellpid_hash;
hashmap_t * multishell_sessions;

/* master and slave pty descriptors */
static int fd_master, fd_slave;

uint16_t term_width     = 80;    /* Width of the terminal (in cells) */
uint16_t term_height    = 25;    /* Height of the terminal (in cells) */
uint16_t csr_x          = 0;    /* Cursor X */
uint16_t csr_y          = 0;    /* Cursor Y */
term_cell_t * term_buffer    = NULL; /* The terminal cell buffer */
uint32_t current_fg     = 7;    /* Current foreground color */
uint32_t current_bg     = 0;    /* Current background color */
uint8_t  cursor_on      = 1;    /* Whether or not the cursor should be rendered */

uint8_t  _login_shell   = 0;    /* Whether we're going to display a login shell or not */
uint8_t  _hold_out      = 0;    /* state indicator on last cell ignore \n */

#define char_width 1
#define char_height 1

term_state_t * ansi_state = NULL;

char * shm_monitor_input;
char * shm_monitor_output;

volatile int exit_application = 0;
unsigned short * textmemptr = (unsigned short *)0xB8000;
#define INPUT_SIZE 1024
char input_buffer[INPUT_SIZE];
int  input_collected = 0;
uint32_t child_pid = 0;

/* ANSI-to-VGA */
char vga_to_ansi[] = {
	0, 4, 2, 6, 1, 5, 3, 7,
	8,12,10,14, 9,13,11,15
};

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

/* Cursor blink timer */
static unsigned int timer_tick = 0;

wchar_t box_chars_in[] = L"▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥▄";
wchar_t box_chars_out[] =  {176,0,0,0,0,248,241,0,0,217,191,218,192,197,196,196,196,196,196,195,180,193,194,179,243,242,220};

void term_clear();

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

static int is_gray(uint32_t a) {
	int a_r = (a & 0xFF0000) >> 16;
	int a_g = (a & 0xFF00) >> 8;
	int a_b = (a & 0xFF);

	return (a_r == a_g && a_g == a_b);
}

static int best_match(uint32_t a) {
	int best_distance = INT32_MAX;
	int best_index = 0;
	for (int j = 0; j < 16; ++j) {
		if (is_gray(a) && !is_gray(vga_base_colors[j])){}
		int distance = color_distance(a, vga_base_colors[j]);
		if (distance < best_distance) {
			best_index = j;
			best_distance = distance;
		}
	}
	return best_index;
}

static void set_term_font_size(float s) {
	/* do nothing */
}

void set_title(char * c) {
	/* Do nothing */
}

void input_buffer_stuff(char * str) {
	unsigned int s = strlen(str) + 1;
	write(fd_master, str, s);
}

void placech(unsigned char c, int x, int y, int attr) {
	unsigned short *where;
	unsigned att = attr << 8;
	where = textmemptr + (y * 80 + x);
	*where = c | att;
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

void term_write_char(uint32_t val, uint16_t x, uint16_t y, uint32_t fg, uint32_t bg, uint8_t flags) {
	if (val > 128)
		val = ununicode(val);
	if (fg > 256)
		fg = best_match(fg);
	if (bg > 256)
		bg = best_match(bg);
	if (fg > 16)
		fg = vga_colors[fg];
	if (bg > 16)
		bg = vga_colors[bg];
	if (fg == 16)
		fg = 0;
	if (bg == 16)
		bg = 0;
	placech(val, x, y, (vga_to_ansi[fg] & 0xF) | (vga_to_ansi[bg] << 4));
}

static void cell_set(uint16_t x, uint16_t y, uint32_t c, uint32_t fg, uint32_t bg, uint8_t flags) {
	if (x >= term_width || y >= term_height) return;
	term_cell_t * cell = (term_cell_t *)((uintptr_t)term_buffer + (y * term_width + x) * sizeof(term_cell_t));
	cell->c     = c;
	cell->fg    = fg;
	cell->bg    = bg;
	cell->flags = flags;
}

static void cell_redraw(uint16_t x, uint16_t y) {
	if (x >= term_width || y >= term_height) return;
	term_cell_t * cell = (term_cell_t *)((uintptr_t)term_buffer + (y * term_width + x) * sizeof(term_cell_t));
	if (((uint32_t *)cell)[0] == 0x00000000)
		term_write_char(' ', x * char_width, y * char_height, TERM_DEFAULT_FG, TERM_DEFAULT_BG, TERM_DEFAULT_FLAGS);
	else
		term_write_char(cell->c, x * char_width, y * char_height, cell->fg, cell->bg, cell->flags);
}

static void cell_redraw_inverted(uint16_t x, uint16_t y) {
	if (x >= term_width || y >= term_height) return;
	term_cell_t * cell = (term_cell_t *)((uintptr_t)term_buffer + (y * term_width + x) * sizeof(term_cell_t));
	if (((uint32_t *)cell)[0] == 0x00000000)
		term_write_char(' ', x * char_width, y * char_height, TERM_DEFAULT_BG, TERM_DEFAULT_FG, TERM_DEFAULT_FLAGS | ANSI_SPECBG);
	else
		term_write_char(cell->c, x * char_width, y * char_height, cell->bg, cell->fg, cell->flags | ANSI_SPECBG);
}

void render_cursor() {
	cell_redraw_inverted(csr_x, csr_y);
}

void draw_cursor() {
	if (!cursor_on) return;
	timer_tick = 0;
	render_cursor();
}

void term_redraw_all() {
	for (uint16_t y = 0; y < term_height; ++y)
		for (uint16_t x = 0; x < term_width; ++x)
			cell_redraw(x,y);
}

void term_scroll(int how_much) {
	if (how_much >= term_height || -how_much >= term_height) {
		term_clear();
		return;
	}

	if (how_much == 0) return;
	if (how_much > 0) {
		/* Shift terminal cells one row up */
		memmove(term_buffer, (void *)((uintptr_t)term_buffer + sizeof(term_cell_t) * term_width), sizeof(term_cell_t) * term_width * (term_height - how_much));
		/* Reset the "new" row to clean cells */
		memset((void *)((uintptr_t)term_buffer + sizeof(term_cell_t) * term_width * (term_height - how_much)), 0x0, sizeof(term_cell_t) * term_width * how_much);
		for (int i = 0; i < how_much; ++i)
			for (uint16_t x = 0; x < term_width; ++x)
				cell_set(x,term_height - how_much,' ', current_fg, current_bg, ansi_state->flags);

		term_redraw_all();
	} else {
		how_much = -how_much;
		/* Shift terminal cells one row up */
		memmove((void *)((uintptr_t)term_buffer + sizeof(term_cell_t) * term_width), term_buffer, sizeof(term_cell_t) * term_width * (term_height - how_much));
		/* Reset the "new" row to clean cells */
		memset(term_buffer, 0x0, sizeof(term_cell_t) * term_width * how_much);
		term_redraw_all();
	}
}

int is_wide(uint32_t codepoint) {
	if (codepoint < 256) return 0;
	return wcwidth(codepoint) == 2;
}

void term_write(char c) {
	static uint32_t codepoint = 0;
	static uint32_t unicode_state = 0;

	cell_redraw(csr_x, csr_y);
	if (!decode(&unicode_state, &codepoint, (uint8_t)c)) {
		if (c == '\r') {
			csr_x = 0;
			return;
		}
		if (csr_x == term_width) {
			csr_x = 0;
			++csr_y;
		}
		if (csr_y == term_height) {
			term_scroll(1);
			csr_y = term_height - 1;
		}
		if (c == '\n') {
			if (csr_x == 0 && _hold_out) {
				_hold_out = 0;
				return;
			}
			++csr_y;
			if (csr_y == term_height) {
				term_scroll(1);
				csr_y = term_height - 1;
			}
			draw_cursor();
		} else if (c == '\007') {
			/* bell */
#if USE_BELL
			for (int i = 0; i < term_height; ++i) {
				for (int j = 0; j < term_width; ++j) {
					cell_redraw_inverted(j, i);
				}
			}
			syscall_nanosleep(0,10);
			term_redraw_all();
#endif
		} else if (c == '\b') {
			if (csr_x > 0)
				--csr_x;
			cell_redraw(csr_x, csr_y);
			draw_cursor();
		} else if (c == '\t') {
			csr_x += (8 - csr_x % 8);
			draw_cursor();
		} else {
			int wide = is_wide(codepoint);
			uint8_t flags = ansi_state->flags;
			if (wide && csr_x == term_width - 1) {
				csr_x = 0;
				++csr_y;
			}
			if (wide)
				flags = flags | ANSI_WIDE;
			cell_set(csr_x,csr_y, codepoint, current_fg, current_bg, flags);
			cell_redraw(csr_x,csr_y);
			csr_x++;
			if (wide && csr_x != term_width) {
				cell_set(csr_x, csr_y, 0xFFFF, current_fg, current_bg, ansi_state->flags);
				cell_redraw(csr_x,csr_y);
				cell_redraw(csr_x-1,csr_y);
				csr_x++;
			}
		}
	} else if (unicode_state == UTF8_REJECT) {
		unicode_state = 0;
	}
	draw_cursor();
}

void term_set_csr(int x, int y) {
	cell_redraw(csr_x,csr_y);
	csr_x = x;
	csr_y = y;
	draw_cursor();
}

int term_get_csr_x() {
	return csr_x;
}

int term_get_csr_y() {
	return csr_y;
}

void term_set_csr_show(uint8_t on) {
	cursor_on = on;
}

void term_set_colors(uint32_t fg, uint32_t bg) {
	current_fg = fg;
	current_bg = bg;
}

void term_redraw_cursor() {
	if (term_buffer)
		draw_cursor();
}

void flip_cursor() {
	static uint8_t cursor_flipped = 0;
	if (cursor_flipped)
		cell_redraw(csr_x, csr_y);
	else
		render_cursor();
	cursor_flipped = 1 - cursor_flipped;
}

void term_set_cell(int x, int y, uint32_t c) {
	cell_set(x, y, c, current_fg, current_bg, ansi_state->flags);
	cell_redraw(x, y);
}

void term_redraw_cell(int x, int y) {
	if (x < 0 || y < 0 || x >= term_width || y >= term_height) return;
	cell_redraw(x,y);
}

void term_clear(int i) {
	if (i == 2) {
		/* Oh dear */
		csr_x = 0;
		csr_y = 0;
		memset((void *)term_buffer, 0x00, term_width * term_height * sizeof(term_cell_t));
		term_redraw_all();
	} else if (i == 0) {
		for (int x = csr_x; x < term_width; ++x)
			term_set_cell(x, csr_y, ' ');

		for (int y = csr_y + 1; y < term_height; ++y)
			for (int x = 0; x < term_width; ++x)
				term_set_cell(x, y, ' ');

	} else if (i == 1) {
		for (int y = 0; y < csr_y; ++y)
			for (int x = 0; x < term_width; ++x)
				term_set_cell(x, y, ' ');

		for (int x = 0; x < csr_x; ++x)
			term_set_cell(x, csr_y, ' ');
	}
}

void clear_input() {
	memset(input_buffer, 0x0, INPUT_SIZE);
	input_collected = 0;
}

void handle_input(char c) {
	write(fd_master, &c, 1);
}

void handle_input_s(char * c) {
	write(fd_master, c, strlen(c));
}

void key_event(int ret, key_event_t * event) {
	if (ret) {
		if ((event->modifiers & KEY_MOD_LEFT_ALT) || (event->modifiers & KEY_MOD_RIGHT_ALT))
			handle_input('\033');
		if (((event->modifiers & KEY_MOD_LEFT_SHIFT) || (event->modifiers & KEY_MOD_RIGHT_SHIFT)) && event->key == '\t') {
				handle_input_s("\033[Z");
				return;
			}
		handle_input(event->key);
	} else {
		if (event->action == KEY_ACTION_UP) return;
		switch (event->keycode) {
			case KEY_F1:
				handle_input_s("\033OP");
				break;
			case KEY_F2:
				handle_input_s("\033OQ");
				break;
			case KEY_F3:
				handle_input_s("\033OR");
				break;
			case KEY_F4:
				handle_input_s("\033OS");
				break;
			case KEY_F5:
				handle_input_s("\033[15~");
				break;
			case KEY_F6:
				handle_input_s("\033[17~");
				break;
			case KEY_F7:
				handle_input_s("\033[18~");
				break;
			case KEY_F8:
				handle_input_s("\033[19~");
				break;
			case KEY_F9:
				handle_input_s("\033[20~");
				break;
			case KEY_F10:
				handle_input_s("\033[21~");
				break;
			case KEY_F11:
				handle_input_s("\033[23~");
				break;
			case KEY_F12:
				handle_input_s("\033[24~");
				break;
			case KEY_ARROW_UP:
				handle_input_s("\033[A");
				break;
			case KEY_ARROW_DOWN:
				handle_input_s("\033[B");
				break;
			case KEY_ARROW_RIGHT:
				handle_input_s("\033[C");
				break;
			case KEY_ARROW_LEFT:
				handle_input_s("\033[D");
				break;
			case KEY_PAGE_UP:
				handle_input_s("\033[5~");
				break;
			case KEY_PAGE_DOWN:
				handle_input_s("\033[6~");
				break;
			case KEY_HOME:
				handle_input_s("\033OH");
				break;
			case KEY_END:
				handle_input_s("\033OF");
				break;
			case KEY_DEL:
				handle_input_s("\e[1C");
				break;
			case KEY_INSERT:
				handle_input_s("\033[2~");
				break;
		}
	}
}

void * wait_for_exit(void * garbage) {
	int pid;
	do {
		pid = waitpid(-1, NULL, 0);
	} while (pid == -1 && errno == EINTR);
	/* Clean up */
	//exit_application = 1;
	/* Exit */
	char exit_message[] = "[Process 'terminal' terminated]\n";
	write(fd_slave, exit_message, sizeof(exit_message));
	pthread_exit(0);
	return NULL;
}

term_callbacks_t term_callbacks = {
	/* writer*/
	&term_write,
	/* set_color*/
	&term_set_colors,
	/* set_csr*/
	&term_set_csr,
	/* get_csr_x*/
	&term_get_csr_x,
	/* get_csr_y*/
	&term_get_csr_y,
	/* set_cell*/
	&term_set_cell,
	/* cls*/
	&term_clear,
	/* scroll*/
	&term_scroll,
	/* redraw_cursor*/
	&term_redraw_cursor,
	/* input_buffer_stuff*/
	&input_buffer_stuff,
	/* set_font_size*/
	&set_term_font_size,
	/* set_title*/
	&set_title,
};

void init_ansi() {
	if (term_buffer) {
		/* Do nothing */
	} else {
		term_buffer = malloc(sizeof(term_cell_t) * term_width * term_height);
		memset(term_buffer, 0x0, sizeof(term_cell_t) * term_width * term_height);
	}

	ansi_state = ansi_init(ansi_state, term_width, term_height, &term_callbacks);
	term_redraw_all();
}

void * handle_incoming(void * garbage) {
	int kbd = open("/dev/kbd", O_RDONLY);
	key_event_t event;
	char c;

	key_event_state_t kbd_state = {0};

	/* Prune any keyboard input we got before the terminal started. */
	struct stat s;
	fstat(kbd, &s);
	for (int i = 0; i < s.st_size; i++) {
		char tmp[1];
		read(kbd, tmp, 1);
	}

	while (!exit_application)
		if (read(kbd, &c, 1) > 0)
			key_event(kbd_scancode(&kbd_state, c, &event), &event);

	pthread_exit(0);
	return NULL;
}

void * blink_cursor(void * garbage) {
	while (!exit_application) {
		timer_tick++;
		if (timer_tick == 3) {
			timer_tick = 0;
			flip_cursor();
		}
		usleep(90000);
	}
	pthread_exit(0);
	return NULL;
}

void outb(uint16_t port, uint16_t data) {
	__asm__ __volatile__("outb %1, %0" : : "dN" (port), "a" (data));
}

static void hide_textmode_cursor() {
	outb(0x3D4, 14);
	outb(0x3D5, 0xFF);
	outb(0x3D4, 15);
	outb(0x3D5, 0xFF);
}

void spawn_shell(int forkno, char * user){
	int shellpid;
	if(!(shellpid = fork())) {
		/* Redirect IO */
		for(int i=0;i<3;i++)
			dup2(fd_slave, STDIN_FILENO + i);

		/* Call shell/login session */
		if(forkno > 0) {
			if(user==NULL) {
				char * tokens[] = {"/bin/login", "-q", NULL};
				execv(tokens[0], tokens);
			} else {
				char * tokens[] = {"/bin/login", "-q", "-u", user, NULL};
				execv(tokens[0], tokens);
			}
		} else {
			char * tokens[] = {"/bin/login", "-u" , "root" , NULL};
			execv(tokens[0], tokens);
		}
	} else {
		char forkno_str[10];
		sprintf(forkno_str, "%d", forkno);
		hashmap_set(shellpid_hash, forkno_str, (void*)shellpid);
		if(user!=NULL)
			hashmap_set(multishell_sessions, user, (void*)shellpid);
	}
}

void update_shm_shmon(int pid){
	char shmon_info[50];
	sprintf(shmon_info, "newshell:%d", pid);
	strcpy(shm_monitor_input, shmon_info);
}

int get_pid_from_hash(int shellno) {
	char shellno_str[5];
	sprintf(shellno_str,"%d", shellno);
	return (int)hashmap_get(shellpid_hash, shellno_str);
}

char * get_username_from_pid(int pid) {
	char * username = NULL;
	list_t * keys = hashmap_keys(multishell_sessions);
	foreach(key,keys)
		if(hashmap_get(multishell_sessions, key->value) == pid) {
			int usr_size = strlen(key->value);
			username = malloc(sizeof(char) * usr_size);
			memset(username, 0, usr_size);
			memcpy(username, key->value, usr_size);
			break;
		}
	list_free(keys);
	free(keys);
	return username;
}

char * read_username_from_shm () {
	int usrname_size = 0;
	int usrname_offset = strchr(shm_monitor_output, ':') - shm_monitor_output;
	for(usrname_size=0; shm_monitor_output[usrname_size+usrname_offset]!='\n' ;usrname_size++);
	char * user = malloc(sizeof(char) * usrname_size);
	memcpy(user, strchr(shm_monitor_output, ':') + 1, usrname_size - 1);
	user[usrname_size-1] = '\0';
	return user;
}

int get_shellno_count() {
	int count = 0;
	list_t * keys = hashmap_keys(shellpid_hash);
	foreach(key, keys)
		count++;
	list_free(keys);
	free(keys);
	return count;
}

void monitor_multishell() {
	shellpid_hash = (hashmap_t*)hashmap_create(MAX_MULTISHELL);
	multishell_sessions = (hashmap_t*)hashmap_create(MAX_MULTISHELL);

	/* Create two shells at startup */
	for(int i=0;i < MULTISHELL_STARTUP_COUNT;i++) {
		spawn_shell(i, NULL);
		shells_forked++;
	}

	/* Setup shared memory: */
	size_t shmon_s = 24;
	/* read only from other processes perspective */
	shm_monitor_input = (char *) syscall_shm_obtain(SHM_SHELLMON_IN, &shmon_s);
	/* write only from other processes perspective */
	shm_monitor_output = (char *) syscall_shm_obtain(SHM_SHELLMON_OUT, &shmon_s);
	memset(shm_monitor_output, 0, shmon_s);
	update_shm_shmon(get_pid_from_hash(shells_forked - 1));

	/* Monitor multishell requests and reap child processes: */
	while(!exit_application) {

		/* Kill the children! Kill them all! */
		int shellcount = get_shellno_count();
		for(int i=0;i < shellcount;i++){
			int child = get_pid_from_hash(i);

			if(waitpid(child, NULL, WNOHANG)>0){ /* One of them died */

				char forkno_str[10];
				sprintf(forkno_str,"%d", i);
				hashmap_remove(shellpid_hash, forkno_str);
				char * user = get_username_from_pid(child);
				hashmap_remove(multishell_sessions, user);
				free(user);

				/* Update the other shellno's onward: */
				for(int j = i;j < shellcount - 1;j++){
					char * forkno_next_str[10];
					sprintf(forkno_str, "%d", j);
					sprintf(forkno_next_str, "%d", (j+1));
					hashmap_set(shellpid_hash, forkno_str, hashmap_get(shellpid_hash,forkno_next_str));
				}
				/* And finally, delete the last one */
				char forkno_last[10];
				sprintf(forkno_last, "%d", shellcount-1);
				hashmap_remove(shellpid_hash, forkno_last);

				shells_forked--;
			}
		}

		/* Use a lock/mutex on the shared memory resource */
		spin_lock(&shm_lock);
		/* Check if there's new requests: */
		if(shm_monitor_output[0]!=0) {
			/* Process request */
			if(shm_monitor_output[0] == '-') { /* We found a command for shmon */
				switch(shm_monitor_output[1]) {
				case SHM_CTRL_GRAB_PID: { /* Allocate shell and return its pid */
					/* Read the user from shared memory */
					char * user = read_username_from_shm();

					/* See if he is already logged (through the hashmap) */
					if(hashmap_has(multishell_sessions, user)) {
						/* return the user's pid instead of the new one (if he's logged in) */
						update_shm_shmon((int)hashmap_get(multishell_sessions, user));
					} else {
						/* the user is not logged in, spawn new shell */
						spawn_shell(shells_forked, user);
						update_shm_shmon(get_pid_from_hash(shells_forked));
						shells_forked++;
					}
					free(user);
					break;
				}
				case SHM_CTRL_ADD_USR: { /* Add a user to the hashmap */
					/* Read the user from shared memory */
					char * user = read_username_from_shm();
					int pid = atoi((char*)strchr(shm_monitor_output, '\n') + 1);

					hashmap_set(multishell_sessions, user, pid);
					free(user);
					break;
				}
				case SHM_CTRL_GET_USR: { /* Get a user from the hashmap */
					/* Grab process pid from user IF he exists */
					/* Read the user from shared memory */
					char * user = read_username_from_shm();
					if(hashmap_has(multishell_sessions, user))
						update_shm_shmon((int)hashmap_get(multishell_sessions, user));
					else
						update_shm_shmon(-1);
					free(user);
					break;
				}
				}
			}

			/* Clear request buffer */
			memset(shm_monitor_output, 0, shmon_s);
		}
		spin_unlock(&shm_lock);
	}

	/* Better not reach this point */

	hashmap_free(multishell_sessions);
	hashmap_free(shellpid_hash);
	free(multishell_sessions);
	free(shellpid_hash);
	syscall_shm_release(SHM_SHELLMON_IN);
	syscall_shm_release(SHM_SHELLMON_OUT);
}

int main(int argc, char ** argv) {
	_login_shell = 0;

	/* Read some arguments */
	static struct option long_opts[] = {
		{"login",      no_argument,       0, 'l'},
		{"help",       no_argument,       0, 'h'},
		{0,0,0,0}
	};

	int index, c;
	while ((c = getopt_long(argc, argv, "hl", long_opts, &index)) != -1) {
		if (!c && long_opts[index].flag == 0)
			c = long_opts[index].val;
		switch (c) {
			case 'l':
				_login_shell = 1;
				break;
			default:
				break;
		}
	}

	hide_textmode_cursor();
	putenv(TERM_ENV);

	syscall_openpty(&fd_master, &fd_slave, NULL, NULL, NULL);
	init_ansi();

	if(!fork())
		monitor_multishell();

	pthread_t wait_for_exit_thread;
	pthread_create(&wait_for_exit_thread, NULL, wait_for_exit, NULL);

	pthread_t handle_incoming_thread;
	pthread_create(&handle_incoming_thread, NULL, handle_incoming, NULL);

	pthread_t cursor_blink_thread;
	pthread_create(&cursor_blink_thread, NULL, blink_cursor, NULL);

	unsigned char buf[1024];
	while (!exit_application) {
		int r = read(fd_master, buf, 1024);
		for (uint32_t i = 0; i < r; ++i)
			ansi_put(ansi_state, buf[i]);
	}

	return 0;
}
