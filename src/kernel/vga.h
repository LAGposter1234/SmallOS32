#pragma once

#include "io/io.h"
#include "fonts/font8x16.h"

#define VGA_WIDTH   640
#define VGA_HEIGHT  480
#define VGA_BYTES   (VGA_WIDTH / 8)
#define VGA_PLANE_SIZE (VGA_BYTES * VGA_HEIGHT)

static uint8_t backbuffer[4][VGA_PLANE_SIZE];

void vga_init(void);

void putpixel(int x, int y, uint8_t color);

void flush(void);

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

void draw_text(const char *s, int x, int y, float size, uint8_t colour);

void clear(uint8_t colour);

void fillrect(int x, int y, int width, int height, uint8_t color);

void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

int draw_image(int x, int y, const uint8_t *data, uint32_t size, float visual_size);
