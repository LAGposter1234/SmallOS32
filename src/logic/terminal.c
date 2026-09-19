#include <stdint.h>
#include "kernel/vga.h"
#include "logic/terminal.h"
#include "fonts/font8x16.h"

#define TERM_COLS 80
#define TERM_ROWS 30

static char terminal[TERM_ROWS][TERM_COLS];

static int cursor_x = 0;
static int cursor_y = 0;

void terminal_clear() {
    for (int y = 0; y < TERM_ROWS; y++) {
        for (int x = 0; x < TERM_COLS; x++) {
            terminal[y][x] = ' ';
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    terminal[cursor_y][cursor_x] = c;
    cursor_x++;

    if (cursor_x >= TERM_COLS) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= TERM_ROWS) {
            terminal_clear();
        }
    }

    if (cursor_y >= TERM_ROWS) {
        terminal_clear();
    }
}

void terminal_draw(void) {
    for (int y = 0; y < TERM_ROWS; y++) {
        for (int x = 0; x < TERM_COLS; x++) {
            const uint8_t *glyph =
            font8x16[(uint8_t)terminal[y][x]];

            for (int row = 0; row < 16; row++) {
                for (int col = 0; col < 8; col++) {
                    uint8_t color = 0;

                    if (glyph[row] & (0x80 >> col) || x == cursor_x && y == cursor_y && row > 14)
                        color = 15;

                    putpixel(
                        x * 8 + col,
                        y * 16 + row,
                        color
                    );
                }
            }
        }
    }
    flush();
}

void terminal_backspace(void) {
    if (cursor_x > 0) {
        cursor_x--;
        terminal[cursor_y][cursor_x] = ' ';
    }
    terminal_draw();
}

void puts(const char *str) {
    while (*str)
        putc(*str++);
}

void fputs(const char *s) {
    puts(s);
    terminal_draw();
}

void fputc(char c) {
    putc(c);
    terminal_draw();
}
