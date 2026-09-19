#include <stdint.h>
#define FONT8x16_IMPLEMENTATION
#include "fonts/font8x16.h"

#include "io/input.h"
#include "io/string.h"
#include "filesystem/fs.h"
#include "io/io.h"

#include "kernel/images.h"

#include "logic/shell.h"
#include "logic/terminal.h"
#include "kernel/vga.h"

#include "kernel/printk.h"


void KERNEL_PANIC(int sync, char* reason) {
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
    // TODO:: NOTICE::: fix FS later PLEASE
    shell();
    for(;;);
}
