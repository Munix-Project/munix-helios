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

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif /* HELIOS_SRC_USR_INCLUDE_GRAPHICS_GRAPHICS_H_ */
