/*
 * start.c
 *
 *  Created on: Sep 12, 2015
 *      Author: miguel
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <graphics/graphics.h>
#include <math.h>

gfx_ctx_t * gfx;

int main(int argc, char** argv) {
	if(!(gfx = init_gfx())) {
		fprintf(stderr, "%s: FATAL - couldn't initialize video! Bailing...\n", argv[0]);
		return 1;
	}

	gfx_draw_fill(gfx, rgb(255,125,50));
	gfx_update(gfx);

	for(;;);

	/* We really shouldn't reach this point ... */
	if(kill_gfx(gfx)) return 1;
	return 0;
}
