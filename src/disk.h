#pragma once
#include <stdint.h>
#include "io.h"

#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECCOUNT   0x1F2
#define ATA_LBA_LO     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE      0x1F6
#define ATA_STATUS     0x1F7
#define ATA_COMMAND    0x1F7

#define ATA_CMD_READ   0x20
#define ATA_CMD_WRITE  0x30

#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERR 0x01

static int ata_wait_bsy(void)
{
    for (uint32_t i = 0; i < 10000000; i++) {
        if (!(inb(ATA_STATUS) & ATA_STATUS_BSY))
            return 0;
    }

    return 1;
}

static uint8_t ata_last_error;

static int ata_wait_drq(void)
{
    for (uint32_t i = 0; i < 10000000; i++) {
        uint8_t status = inb(ATA_STATUS);

        if (status & ATA_STATUS_ERR) {
            ata_last_error = inb(ATA_ERROR);
            return 1;
        }

        if (!(status & ATA_STATUS_BSY) &&
            (status & ATA_STATUS_DRQ)) {
            return 0;
            }
    }

    ata_last_error = 0xFF; // timeout
    return 1;
}

static void ata_400ns_delay(void)
{
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
}

int disk_read_sector(uint32_t lba, void *buffer) {
    uint16_t *buf = (uint16_t *)buffer;

    ata_wait_bsy();

    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    ata_400ns_delay();
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO,  lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HI,  (lba >> 16) & 0xFF);

    outb(ATA_COMMAND, ATA_CMD_READ);

    if (ata_wait_drq())
        return 1;

    for (int i = 0; i < 256; i++)
        buf[i] = inw(ATA_DATA);

    if (ata_wait_bsy())
        return 1;

    if (inb(ATA_STATUS) & ATA_STATUS_ERR)
        return 1;

    return 0;
}

int disk_write_sector(uint32_t lba, const void *buffer) {
    const uint16_t *buf = (const uint16_t *)buffer;

    ata_wait_bsy();

    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    ata_400ns_delay();
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO,  lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HI,  (lba >> 16) & 0xFF);

    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (ata_wait_drq())
        return 1;

    for (int i = 0; i < 256; i++)
        outw(ATA_DATA, buf[i]);

    if (ata_wait_bsy())
        return 1;

    if (inb(ATA_STATUS) & ATA_STATUS_ERR)
        return 1;

    return 0;
}
