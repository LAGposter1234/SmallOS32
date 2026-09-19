#pragma once
#include <stdint.h>

int disk_read_sector(uint32_t lba, void *buffer);
int disk_write_sector(uint32_t lba, const void *buffer);

int disk_test_integrity(void);
