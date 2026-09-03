#pragma once
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#define FILE_CAPACITY ((1024 * 1024) * 1)
#define MAX_FILES 32

typedef struct {
    uint8_t free;
    char name[32];
    uint32_t size;
    uint8_t data[FILE_CAPACITY];
} fs_file;

static fs_file files[MAX_FILES];

fs_file* fs_open(char name[32]) {
    // simple linear search
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].free) continue;
        if (strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

int fs_write(fs_file *file) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].free) continue;

        if (strcmp(files[i].name, file->name) == 0) {
            files[i] = *file;
            return 0;
        }
    }

    return 1;
}

fs_file *fs_makef(char name[32], uint8_t *buf, uint32_t size) {
    if (size > FILE_CAPACITY)
        return NULL;

    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].free)
            continue;

        files[i].free = 0;
        strcpy(files[i].name, name, 32);
        files[i].size = size;
        memcpy(files[i].data, buf, size);

        return &files[i];
    }

    return NULL;
}


void fs_list(char buf[1024]) {
    char tmp[32];
    char tmp2[16];
    buf[0] = '\0';
    strappend(buf, "SFS2 filesystem\nCapacity: ");

    itoa(FILE_CAPACITY, tmp);
    strappend(buf, tmp);

    int filecount = 0;
    for (int i = 0; i < 32; i++) {
        if (files[i].free) continue;
        filecount++;
    }

    itoa(filecount, tmp2);
    strappend(buf, "\nCount: ");
    strappend(buf, tmp2);

    strappend(buf, "\nFiles: \n");

    for(int i = 0; i < MAX_FILES; i++) {
        if(files[i].free) continue;
        itoa(i, tmp2);
        strappend(buf, tmp2);
        strappend(buf, " ");

        strappend(buf, files[i].name);

        strappend(buf, " ");
        itoa(files[i].size, tmp);
        strappend(buf, tmp);

        strappend(buf, "\n");
    }
    strappend(buf, "\n");
}

void fs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].free = 1;
    }
}
