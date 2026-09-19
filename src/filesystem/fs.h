#pragma once
#include <stdint.h>
#include <stddef.h>
#include "io/string.h"
#include "disk/disk.h"

#define FILE_CAPACITY ((1024 * 1024) * 1)
#define MAX_FILES 32
#define FS_START_SECTOR 40
#define FS_SECTOR_SIZE 512

#define FS_SIZE_BYTES   sizeof(files)
#define FS_SECTOR_COUNT ((FS_SIZE_BYTES + FS_SECTOR_SIZE - 1) / FS_SECTOR_SIZE)

typedef struct __attribute__((packed)) {
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

#define FS_META_SIZE 37

int fs_sync(void) {
    uint8_t *ptr = (uint8_t *)files;
    uint8_t buf[FS_SECTOR_SIZE];
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        uint32_t offset = i * sizeof(fs_file);
        uint32_t sector = FS_START_SECTOR + offset / FS_SECTOR_SIZE;
        uint32_t off = offset % FS_SECTOR_SIZE;
        if (disk_read_sector(sector, buf) != 0) return 1;
        memcpy(buf + off, ptr + offset, FS_META_SIZE);
        if (disk_write_sector(sector, buf) != 0) return 1;
        if (files[i].free) continue;
        uint32_t data_offset = offset + FS_META_SIZE;
        uint32_t remaining = files[i].size;
        uint32_t src = 0;
        while (remaining) {
            sector = FS_START_SECTOR + data_offset / FS_SECTOR_SIZE;
            off = data_offset % FS_SECTOR_SIZE;
            if (disk_read_sector(sector, buf) != 0) return 1;
            uint32_t n = FS_SECTOR_SIZE - off;
            if (n > remaining) n = remaining;
            memcpy(buf + off, files[i].data + src, n);
            if (disk_write_sector(sector, buf) != 0) return 1;
            data_offset += n;
            src += n;
            remaining -= n;
        }
    }
    return 0;
}

int fs_reload(void) {
    uint8_t *ptr = (uint8_t *)files;
    for (uint32_t i = 0; i < MAX_FILES; i++) {
        uint32_t offset = i * sizeof(fs_file);
        uint32_t sector = FS_START_SECTOR + offset / FS_SECTOR_SIZE;
        uint32_t off = offset % FS_SECTOR_SIZE;
        uint8_t buf[FS_SECTOR_SIZE];
        if (disk_read_sector(sector, buf) != 0) return 1;
        memcpy(ptr + offset, buf + off, FS_META_SIZE);
        if (files[i].free) continue;
        uint32_t data_offset = offset + FS_META_SIZE;
        uint32_t remaining = files[i].size;
        uint32_t dest = 0;
        while (remaining) {
            sector = FS_START_SECTOR + data_offset / FS_SECTOR_SIZE;
            off = data_offset % FS_SECTOR_SIZE;
            if (disk_read_sector(sector, buf) != 0) return 1;
            uint32_t n = FS_SECTOR_SIZE - off;
            if (n > remaining)n = remaining;
            memcpy(files[i].data + dest, buf + off, n);
            data_offset += n;
            dest += n;
            remaining -= n;
        }
    }
    return 0;
}

int fs_format(void) {
    fs_init();
    return fs_sync();
}
