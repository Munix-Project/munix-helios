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

int fd, status;

void reset_graphics() {
	if (ioctl(fd, 0x5005, &status) == -1)
		printf("IO_VID_RST failed: %s\n", strerror(errno));
}

void start() {
	fd = open("/dev/fb0", O_WRONLY);
	reset_graphics();
}

void stop() {
	close(fd);
}

int main(int argc, char** argv) {
	start();
	if (ioctl(fd, IO_VID_RST, &status) == -1)
		printf("IO_VID_RST failed: %s\n", strerror(errno));

	stop();
	return 0;
}
