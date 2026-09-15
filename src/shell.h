#pragma once

#include "terminal.h"


int shell_exec(char *script, uint32_t size);
int handle_cmd(char args[32][128]);
int try_exec(char *name);

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
    } else if (strcmp(args[0], "exit") == 0) {
        return 1000 + parse_int(args[1]);
    } else {
        fputs("Oops");
        return 1001;
    }
    end:
    flush();
    return 0;
}

int try_exec(char *name) {
    fs_file *file = fs_open(name);
    if (!file) return 1;
    return shell_exec((char*)file->data, file->size);
}


int shell_exec(char *script, uint32_t size) {
    char line[128];
    uint32_t pos = 0;
    uint32_t len;

    while (pos < size) {
        len = 0;

        while (pos + len < size &&
            script[pos + len] != '\n' &&
            len < sizeof(line) - 1)
            len++;

        for (uint32_t i = 0; i < len; i++)
            line[i] = script[pos + i];

        line[len] = 0;

        if (len) {
            char split_cmd[32][128];
            split(line, split_cmd);
            int status = handle_cmd(split_cmd) - 1000;
            /*
             * This is where some magic happens
             * handle_command() may encounter "exit 1" or "exit 0",
             * in that case, it will return the exit code + 1000.
             * If not, status will be negative and we will know
             * its nothing to worry about.
            */
            if (status >= 0) return status;
            /*
             * There is still a problem with this design, however.
             * A program may do "exit -1", which would be bad.
             * In that case, however, we can just error and
             * return 1001.
            */
        }
        pos += len;

        if (pos < size && script[pos] == '\n')
            pos++;
    }
}
