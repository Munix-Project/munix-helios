/*
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2015 Dale Weiler
 *
 */

#include <graphics/graphics.h>
#include <stdint.h>
#include <drivers/gpu/video.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return (a * 0x1000000) + (r * 0x10000) + (g * 0x100) + (b * 0x1);
}

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
	return 0xFF000000 + (r * 0x10000) + (g * 0x100) + (b * 0x1);
}

gfx_ctx_t * init_gfx() {
	if(vid_start()) return NULL;

	Point3 screen_size = vid_get_size();
	if(screen_size.X == -1) return NULL;

	char * buff;
	if(!(buff = vid_get_mem())) return NULL;

	gfx_ctx_t * ret = malloc(sizeof(gfx_ctx_t));
	ret->width 		= screen_size.X;
	ret->height 	= screen_size.Y;
	ret->depth 		= screen_size.Z;
	ret->sizeint 	= CTX_H(ret) * CTX_W(ret) * CTX_B(ret);
	ret->size.X 	= ret->width;
	ret->size.Y 	= ret->height;
	ret->size.Z 	= ret->depth;
	ret->buff 		= buff;
	ret->bbuff 		= malloc(sizeof(uint32_t) * CTX_H(ret) * CTX_W(ret));

	return ret;
}

int kill_gfx(gfx_ctx_t * ctx) {
	/* TODO: Kill more things! */
	free(ctx ->bbuff);
	free(ctx);
	return vid_stop();
}

void gfx_flip(gfx_ctx_t * ctx) {
	memcpy(ctx->buff, ctx->bbuff, ctx->sizeint);
}

void gfx_clear(gfx_ctx_t * ctx) {
	memset(ctx->bbuff, 0, ctx->sizeint);
}

void gfx_update(gfx_ctx_t * ctx) {
	/* same as gx_flip */
	gfx_flip(ctx);
}

/*********** lfbvideo firmware ***********/

int vid_fd, status;

int vid_reset_graphics() {
	if (ioctl(vid_fd, IO_VID_RST, &status) == -1) {
		fprintf(stderr, "IO_VID_RST failed: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

int vid_start() {
	vid_fd = open("/dev/fb0", O_RDWR);
	return vid_reset_graphics();
}

int vid_stop() {
	if (ioctl(vid_fd, IO_VID_STP, &status) == -1) {
		fprintf(stderr, "IO_VID_STP failed: %s\n", strerror(errno));
		return 1;
	}
	close(vid_fd);
	return 0;
}

int vid_resize(int width, int height, int depth) {
	Point3 * newsize = malloc(sizeof(Point3));
	newsize->X = width;
	newsize->Y = height;
	newsize->Z = depth;
	if (ioctl(vid_fd, IO_VID_RESZ, newsize) == -1) {
		fprintf(stderr, "IO_VID_RESZ failed: %s\n", strerror(errno));
		free(newsize);
		return 1;
	}
	free(newsize);
	return 0;
}

Point3 vid_get_size() {
	/* Fetch size with pointer */
	Point3 * size_ptr = malloc(sizeof(Point3));
	if (ioctl(vid_fd, IO_VID_GETSZ, size_ptr) == -1) {
		fprintf(stderr, "IO_VID_RESZ failed: %s\n", strerror(errno));
		free(size_ptr);
		/* Return invalid size: */
		Point3 size;
		size.X = size.Y = size.Z = -1;
		return size;
	}

	/* And prepare result: */
	Point3 size;
	size.X = size_ptr->X;
	size.Y = size_ptr->Y;
	size.Z = size_ptr->Z;
	free(size_ptr);
	return size;
}

char * vid_get_mem() {
	char* ptr;
	if(ioctl(vid_fd, IO_VID_ADDR, &ptr) == -1) {
		fprintf(stderr, "IO_VID_ADDR failed: %s\n", strerror(errno));
		return NULL;
	}
	return ptr;
}

/******************* actual graphical/drawing functions *******************/
void gfx_draw_fill(gfx_ctx_t * ctx, uint32_t color) {
	for (uint16_t y = 0; y < ctx->height; ++y)
		for (uint16_t x = 0; x < ctx->width; ++x)
			GFX(ctx, x, y) = color;
}

void gfx_clear_screen(gfx_ctx_t * ctx) {
	gfx_draw_fill(ctx, rgb(0, 0, 0));
}

