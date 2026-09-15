#pragma once
#include "io.h"

static const char scancode_to_ascii[128] = {
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',

    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',

    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',

    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',

    [0x1C] = '\n',  // Enter
    [0x0E] = '\b',  // Backspace
    [0x39] = ' ',   // Space
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x0C] = '-'
};

char getch(void) {
    uint8_t scancode;

    for (;;) {
        while (!(inb(0x64) & 1))
            ;

        scancode = inb(0x60);

        // Ignore key releases
        if (scancode & 0x80)
            continue;

        if (scancode_to_ascii[scancode])
            return scancode_to_ascii[scancode];
    }
}


char getch_nonblock(void) {
    uint8_t scancode;

    if (!(inb(0x64) & 1))
        return 0;

    scancode = inb(0x60);

    if (scancode & 0x80)
        return 0;

    if (scancode_to_ascii[scancode])
        return scancode_to_ascii[scancode];

    return 0;
}


void fputc(char c);
void putc(char c);
void flush(void);
void terminal_backspace(void);
static int cursor_x;
static int cursor_y;

char* get_line(void) {
    char c;
    static char str[128];
    int pos = 0;

    while(1) {
        c = getch();
        if (c == '\b') {
            if (pos > 0) {
                pos--;
                str[pos] = '\0';
                terminal_backspace();
                flush();
            }
        } else if (c == '\n') {
            fputc('\n');
            return str;
        } else {
            str[pos] = c;
            pos++;
            fputc(c);
        }
    }
}
