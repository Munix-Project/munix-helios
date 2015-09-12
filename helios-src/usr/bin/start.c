/*
 * start.c
 *
 *  Created on: Sep 12, 2015
 *      Author: miguel
 */

#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <drivers/gpu/video.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

int vid_fd, status;

int reset_graphics() {
	if (ioctl(vid_fd, IO_VID_RST, &status) == -1) {
		fprintf(stderr, "IO_VID_RST failed: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

int vid_start() {
	vid_fd = open("/dev/fb0", O_WRONLY);
	return reset_graphics();
}

int vid_stop() {
	if (ioctl(vid_fd, IO_VID_STP, &status) == -1) {
		fprintf(stderr, "IO_VID_STP failed: %s\n", strerror(errno));
		return 1;
	}
	close(vid_fd);
	return 0;
}

/*
 * Macros make verything easier.
 */

#define GFX(ctx,x,y) *((uint32_t *)&((ctx))[(800 * (y) + (x)) * 4])

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
	return 0xFF000000 + (r * 0x10000) + (g * 0x100) + (b * 0x1);
}

int main(int argc, char** argv) {
	if(vid_start()) return 1;

	char* ptr;
	ioctl(vid_fd, IO_VID_ADDR, &ptr);


	for (uint16_t y = 0; y < 600; y++) {
		for (uint16_t x = 0; x < 400; x++) {
			if(y<300)
				GFX(ptr,x,y) = rgb(255, 255, 0);
			else
				GFX(ptr,x,y) = rgb(255, 0, 0);

		}
	}

	for (uint16_t y = 0; y < 600; y++) {
		for (uint16_t x = 400; x < 800; x++) {
			if(y<300)
				GFX(ptr,x,y) = rgb(0, 255, 0);
			else
				GFX(ptr,x,y) = rgb(0, 0, 255);

		}
	}

	for(;;);
	if(vid_stop()) return 1;
	return 0;
}
