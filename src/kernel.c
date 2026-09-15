#include <stdint.h>
#define FONT8x16_IMPLEMENTATION
#include "font8x16.h"
#include "input.h"
#include "string.h"
#include "fs.h"
#include "io.h"

#include "syscalls.h"

#include "nosignalimg.h"
#include "scratch.h"

#include "shell.h"
#include "terminal.h"

#include "vga.h"

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

void shell(void) {
    char args[32][128];
    int last_output = 0;

    for(;;) {
        fputs("# ");
        char* input = get_line();

        split(input, args);
        last_output = handle_cmd(args);
        memset(input, 0, 128);
    }
}


void KERNEL_PANIC(int sync, char* reason) {
    terminal_clear();
    if (!sync) fputs("Not syncing: ");
    fputs("KERNEL PANIC!\n");
    fputs(reason);
    __asm__ volatile("cli; hlt");
}
void draw_image_rle(int x, int y, const uint8_t *data, uint32_t size, int width, int height) {
    uint32_t pos = 0;
    uint32_t pixel = 0;

    while (pos + 1 < size && pixel < 320 * 240) {
        uint8_t count = data[pos++];
        uint8_t color = data[pos++];

        for (uint32_t i = 0; i < count && pixel < 320 * 240; i++) {
            int px = pixel % 320;
            int py = pixel / 320;

            int dx = (px * width) / 320;
            int dy = (py * height) / 240;

            putpixel(x + dx, y + dy, color);

            pixel++;
        }
    }
}

int try_exec(char *name) {
    fs_file *file = fs_open(name);
    if (!file) return 1;
    return shell_exec(file->data, file->size);
}

__attribute__((section(".text.entry")))
void kernel_main(void) {
    terminal_clear();
    vga_init();
    fputs("VGA Init\n");
    fs_init();
    fault_init();
    fputs("Syscall Init\n");

    try_exec("init");

    KERNEL_PANIC(0, "Init was killed!");
}
