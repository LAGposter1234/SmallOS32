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
