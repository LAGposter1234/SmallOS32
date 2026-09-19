#include <disk/disk.h>
#include <kernel/printk.h>

#define DISK_TEST_SECTOR 10000

int disk_test_integrity(void) {
    uint8_t original[512];
    uint8_t write_buf[512];
    uint8_t read_buf[512];
    if (disk_read_sector(DISK_TEST_SECTOR, original))
        return 1;
    for (uint32_t i = 0; i < 512; i++)
        write_buf[i] = i;

    if (disk_write_sector(DISK_TEST_SECTOR, write_buf))
        return 1;
    if (disk_read_sector(DISK_TEST_SECTOR, read_buf))
        return 1;

    for (uint32_t i = 0; i < 512; i++) {
        if (read_buf[i] != write_buf[i]) {
            return 1;
        }
    }
    if (disk_write_sector(DISK_TEST_SECTOR, original))
        return 1;
    return 0;
}
