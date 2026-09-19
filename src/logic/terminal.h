#pragma once

#include "kernel/vga.h"
#define TERM_COLS 80
#define TERM_ROWS 30

static char terminal[TERM_ROWS][TERM_COLS];

static int cursor_x;
static int cursor_y;

void terminal_clear();
void putc(char c);
void terminal_draw(void);
void terminal_backspace(void);
void puts(const char *str);
void fputs(const char *s);
void fputc(char c);
