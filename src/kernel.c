#include <stdint.h>
#define FONT8x16_IMPLEMENTATION
#include "font8x16.h"

#include "input.h"
#include "string.h"
#include "fs.h"
#include "io.h"

#include "nosignalimg.h"
#include "scratch.h"

#include "shell.h"
#include "terminal.h"

#include "vga.h"


void KERNEL_PANIC(int sync, char* reason) {
    terminal_clear();
    if (!sync) fputs("Not syncing: ");
    fputs("KERNEL PANIC!\n");
    fputs(reason);
    __asm__ volatile("cli; hlt");
}

__attribute__((section(".text.entry")))
void kernel_main(void) {
    terminal_clear();
    vga_init();
    fputs("VGA Init\n");
    fs_init();

    try_exec("init");

    KERNEL_PANIC(0, "Init was killed!");
    // how would we ever get here
    for(;;);
}
