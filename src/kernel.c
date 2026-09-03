#include <stdint.h>
#define FONT8x16_IMPLEMENTATION
#include "font8x16.h"
#include "input.h"
#include "string.h"
#include "fs.h"

#define VGA_WIDTH   640
#define VGA_HEIGHT  480
#define VGA_BYTES   (VGA_WIDTH / 8)
#define VGA_PLANE_SIZE (VGA_BYTES * VGA_HEIGHT)

static uint8_t backbuffer[4][VGA_PLANE_SIZE];

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

#define TERM_COLS 80
#define TERM_ROWS 30

static char terminal[TERM_ROWS][TERM_COLS];

void vga_init(void) {
    // Sequencer: enable all 4 planes
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x0F);

    // Data rotate = 0
    outb(0x3CE, 0x03);
    outb(0x3CF, 0x00);

    // Write mode 0
    outb(0x3CE, 0x05);
    outb(0x3CF, 0x00);

    // Set/Reset
    outb(0x3CE, 0x00);
    outb(0x3CF, 0x00);

    // Enable Set/Reset
    outb(0x3CE, 0x01);
    outb(0x3CF, 0x0F);

    // Bit mask = all bits
    outb(0x3CE, 0x08);
    outb(0x3CF, 0xFF);
}

void putpixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
        return;

    uint32_t offset = y * VGA_BYTES + (x >> 3);
    uint8_t mask = 0x80 >> (x & 7);

    for (int plane = 0; plane < 4; plane++) {
        if (color & (1 << plane))
            backbuffer[plane][offset] |= mask;
        else
            backbuffer[plane][offset] &= ~mask;
    }
}

void flush(void) {
    // Disable Set/Reset
    outb(0x3CE, 0x01);
    outb(0x3CF, 0x00);

    // Bit mask = all bits
    outb(0x3CE, 0x08);
    outb(0x3CF, 0xFF);

    volatile uint8_t *vram = (volatile uint8_t *)0xA0000;

    for (int plane = 0; plane < 4; plane++) {
        // Select plane
        outb(0x3C4, 0x02);
        outb(0x3C5, 1 << plane);

        for (uint32_t i = 0; i < VGA_PLANE_SIZE; i++)
            vram[i] = backbuffer[plane][i];
    }

    // Re-enable all planes
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x0F);

    // Restore Set/Reset state for putpixel()
    outb(0x3CE, 0x01);
    outb(0x3CF, 0x0F);
}

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

static int cursor_x = 0;
static int cursor_y = 0;

void terminal_clear() {
    for (int y = 0; y < TERM_ROWS; y++) {
        for (int x = 0; x < 80; x++) {
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

void clear(uint8_t colour) {
    for (int x = 0; x < 640; x++) {
        for (int y = 0; y < 480; y++) {
            putpixel(x, y, colour);
        }
    }
}

void fillrect(int x, int y, int width, int height, uint8_t color) {
    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            putpixel(px, py, color);
        }
    }
}

void demo() {
    clear(3); // desktop background

    // taskbar
    fillrect(0, 440, 640, 40, 8);

    // start button
    fillrect(4, 444, 60, 32, 7);

    // button highlight
    fillrect(4, 444, 60, 2, 15);
    fillrect(4, 444, 2, 32, 15);

    // button shadow
    fillrect(4, 474, 60, 2, 0);
    fillrect(62, 444, 2, 32, 0);

    // fake window
    fillrect(120, 80, 400, 280, 7);

    // title bar
    fillrect(120, 80, 400, 20, 1);

    // window border
    fillrect(120, 80, 400, 2, 15);
    fillrect(120, 80, 2, 280, 15);
    fillrect(120, 358, 400, 2, 0);
    fillrect(518, 80, 2, 280, 0);

    flush();

    for (;;);
}

int handle_cmd(char args[32][128]) {
    if (strcmp(args[0], "echo") == 0) {
        puts(args[1]);
        putc('\n');
    } else if (strcmp(args[0], "clear") == 0) {
        terminal_clear();
    } else if (strcmp(args[0], "demogui") == 0) {
        demo();
    } else if (strcmp(args[0], "ls") == 0) {
        char buf[1024];
        fs_list(buf);
        puts(buf);
    } else if (strcmp(args[0], "cat") == 0) {
        fs_file *file = fs_open(args[1]);
        if (file == NULL) {
            puts("fym ");
            puts(args[1]);
            puts("\n");
            goto end;
        }
        for (uint32_t i = 0; i < file->size; i++) {
            putc(file->data[i]);
        }
    }
    end:
    flush();
    return 0;
}

__attribute__((section(".text.entry")))
void kernel_main(void) {
    vga_init();
    fputs("VGA Init\n");
    fs_init();
    fputs("SFS2 Init\n");
    fputs("Welcome to SmallOS32.\n");
    char name[32] = "hello.txt";
    fs_makef(name, "Hello, World!\n", 14);

    // shell

    char args[32][128];
    int last_output = 0;

    for(;;) {
        fputs("# ");
        char* input = get_line();

        split(input, args);
        // fputs(args[0]);
        last_output = handle_cmd(args);
        memset(input, 0, 128);
    }
}
