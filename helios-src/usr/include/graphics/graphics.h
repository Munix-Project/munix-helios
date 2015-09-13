/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
 */

#ifndef HELIOS_SRC_USR_INCLUDE_GRAPHICS_GRAPHICS_H_
#define HELIOS_SRC_USR_INCLUDE_GRAPHICS_GRAPHICS_H_

#include <syscall.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Macros for applying graphics */

#define CTX_W(ctx) ((ctx)->width)		/* Display width */
#define CTX_H(ctx) ((ctx)->height)		/* Display height */
#define CTX_B(ctx) ((ctx)->depth / 8)	/* Display byte depth */

#define GFX(ctx, x, y) *((uint32_t *)&((ctx)->bbuff)[(CTX_W(ctx) * y + x) * CTX_B(ctx)])

typedef struct gfx_ctx {
	uint16_t width;
	uint16_t height;
	uint16_t depth;
	Point3 size;
	uint32_t sizeint;
	char * buff;
	char * bbuff;
} gfx_ctx_t;

/* init and uninitialize functions: */
extern gfx_ctx_t * init_gfx();
extern int kill_gfx(gfx_ctx_t * ctx);

extern void gfx_flip(gfx_ctx_t * ctx);
extern void gfx_clear(gfx_ctx_t * ctx);
extern void gfx_update(gfx_ctx_t * ctx);

extern uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
extern uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/*********** lfbvideo firmware ***********/

extern int vid_reset_graphics();
extern int vid_start();
extern int vid_stop();
extern int vid_resize(int width, int height, int depth);
extern Point3 vid_get_size();
extern char * vid_get_mem();

/******************* actual graphical/drawing functions *******************/
extern void gfx_draw_fill(gfx_ctx_t * ctx, uint32_t color);
extern void gfx_clear_screen(gfx_ctx_t * ctx);
#endif /* HELIOS_SRC_USR_INCLUDE_GRAPHICS_GRAPHICS_H_ */
