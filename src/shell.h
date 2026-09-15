#pragma once

#include "terminal.h"

int handle_cmd(char args[32][128]) {
    if (strcmp(args[0], "echo") == 0) {
        puts(args[1]);
        putc('\n');
    } else if (strcmp(args[0], "clear") == 0) {
        terminal_clear();
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
    } else if (strcmp(args[0], "sync") == 0) {
        fs_sync();
    } else if (strcmp(args[0], "format") == 0) {
        fs_format();
    } else {
        fs_file* file = fs_open(args[0]);
        if (file) {
            __asm__ volatile (
                "jmp *%0"
                :
                : "r"(file->data)
                : "memory"
            );
        } else {
            puts("No such file\n");
        }
    }
    end:
    flush();
    return 0;
}
