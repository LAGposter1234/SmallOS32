#pragma once

#define VGA_WIDTH   640
#define VGA_HEIGHT  480
#define VGA_BYTES   (VGA_WIDTH / 8)
#define VGA_PLANE_SIZE (VGA_BYTES * VGA_HEIGHT)

static uint8_t backbuffer[4][VGA_PLANE_SIZE];

void vga_init(void) {
    // Sequencer: enable all 4 planes
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x0F);

    // Data rotate = 0
    outb(0x3CE, 0x03);
    outb(0x3CF, 0x00);

    // Write mode 0
    outb(0x3CE, 0x05);
    outb(0x3CF, 0x00);

    // Set/Reset
    outb(0x3CE, 0x00);
    outb(0x3CF, 0x00);

    // Enable Set/Reset
    outb(0x3CE, 0x01);
    outb(0x3CF, 0x0F);

    // Bit mask = all bits
    outb(0x3CE, 0x08);
    outb(0x3CF, 0xFF);
}

void putpixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
        return;

    uint32_t offset = y * VGA_BYTES + (x >> 3);
    uint8_t mask = 0x80 >> (x & 7);

    for (int plane = 0; plane < 4; plane++) {
        if (color & (1 << plane))
            backbuffer[plane][offset] |= mask;
        else
            backbuffer[plane][offset] &= ~mask;
    }
}

void flush(void) {
    // Disable Set/Reset
    outb(0x3CE, 0x01);
    outb(0x3CF, 0x00);

    // Bit mask = all bits
    outb(0x3CE, 0x08);
    outb(0x3CF, 0xFF);

    volatile uint8_t *vram = (volatile uint8_t *)0xA0000;

    for (int plane = 0; plane < 4; plane++) {
        // Select plane
        outb(0x3C4, 0x02);
        outb(0x3C5, 1 << plane);

        for (uint32_t i = 0; i < VGA_PLANE_SIZE; i++)
            vram[i] = backbuffer[plane][i];
    }

    // Re-enable all planes
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x0F);

    // Restore Set/Reset state for putpixel()
    outb(0x3CE, 0x01);
    outb(0x3CF, 0x0F);
}

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

void draw_text(const char *s, int x, int y, float size, uint8_t colour) {
    int integer = (int)size;

    if (size == integer) {
        for (int i = 0; s[i]; i++) {
            const uint8_t *glyph = font8x16[(uint8_t)s[i]];

            for (int row = 0; row < 16; row++) {
                for (int col = 0; col < 8; col++) {
                    if (!(glyph[row] & (0x80 >> col)))
                        continue;

                    for (int sy = 0; sy < integer; sy++)
                        for (int sx = 0; sx < integer; sx++)
                            putpixel(
                                x + i * 8 * integer + col * integer + sx,
                                y + row * integer + sy,
                                colour
                            );
                }
            }
        }
        return;
    }

    for (int i = 0; s[i]; i++) {
        const uint8_t *glyph = font8x16[(uint8_t)s[i]];

        int w = (int)(8 * size);
        int h = (int)(16 * size);

        for (int py = 0; py < h; py++) {
            for (int px = 0; px < w; px++) {
                float gx = px / size;
                float gy = py / size;

                int x0 = (int)gx;
                int y0 = (int)gy;
                int x1 = x0 + 1;
                int y1 = y0 + 1;

                float fx = gx - x0;
                float fy = gy - y0;

                int p00 = x0 < 8 && y0 < 16 &&
                (glyph[y0] & (0x80 >> x0));
                int p10 = x1 < 8 && y0 < 16 &&
                (glyph[y0] & (0x80 >> x1));
                int p01 = x0 < 8 && y1 < 16 &&
                (glyph[y1] & (0x80 >> x0));
                int p11 = x1 < 8 && y1 < 16 &&
                (glyph[y1] & (0x80 >> x1));

                float v =
                p00 * (1 - fx) * (1 - fy) +
                p10 * fx       * (1 - fy) +
                p01 * (1 - fx) * fy +
                p11 * fx       * fy;

                if (v > 0.5f)
                    putpixel(
                        x + i * w + px,
                        y + py,
                        colour
                    );
            }
        }
    }
}

void clear(uint8_t colour) {
    uint8_t mask = 0xFF;

    for (int plane = 0; plane < 4; plane++) {
        if (!(colour & (1 << plane)))
            mask = 0;

        for (int i = 0; i < VGA_PLANE_SIZE; i++)
            backbuffer[plane][i] = mask;
    }
}

void fillrect(int x, int y, int width, int height, uint8_t color) {
    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            putpixel(px, py, color);
        }
    }
}

void vga_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r >> 2);
    outb(0x3C9, g >> 2);
    outb(0x3C9, b >> 2);
}

void draw_image_rle(int x, int y, const uint8_t *data, uint32_t size, int width, int height) {
    uint32_t pos = 0;
    uint32_t pixel = 0;

    while (pos + 1 < size && pixel < 320 * 240) {
        uint8_t count = data[pos++];
        uint8_t color = data[pos++];

        for (uint32_t i = 0; i < count && pixel < 320 * 240; i++) {
            int px = pixel % 320;
            int py = pixel / 320;

            int dx = (px * width) / 320;
            int dy = (py * height) / 240;

            putpixel(x + dx, y + dy, color);

            pixel++;
        }
    }
}
