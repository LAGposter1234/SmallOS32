#include "io/io.h"
#include "fonts/font8x16.h"
#include "kernel/vga.h"

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

int draw_image(int x, int y, const uint8_t *data, uint32_t size, float visual_size) {
    if (size < 8 || data[0] != 'K' || data[1] != 'M' || data[2] != 'G')
        return 1;

    uint16_t width = data[3] | ((uint16_t)data[4] << 8);
    uint16_t height = data[5] | ((uint16_t)data[6] << 8);
    uint8_t bpp = data[7];

    if (!width || !height || visual_size <= 0.0f)
        return 1;

    if (bpp != 1 && bpp != 2 && bpp != 4)
        return 1;

    if (visual_size >= 255.0f) {
        float sx = 640.0f / width;
        float sy = 480.0f / height;
        visual_size = sx < sy ? sx : sy;
    }

    uint8_t row[width];
    uint32_t pos = 8;
    uint32_t run_count = 0;
    uint8_t run_value = 0;

    for (uint32_t sy = 0; sy < height; sy++) {
        if (bpp == 1 || bpp == 2) {
            uint32_t per_byte = 8 / bpp;
            uint32_t packed_width = (width + per_byte - 1) / per_byte;
            uint32_t px = 0;

            for (uint32_t p = 0; p < packed_width; p++) {
                if (!run_count) {
                    if (pos + 2 > size)
                        return 1;

                    run_count = data[pos++];
                    run_value = data[pos++];

                    if (!run_count)
                        return 1;
                }

                uint8_t packed = run_value;
                run_count--;

                for (uint32_t i = 0; i < per_byte && px < width; i++, px++) {
                    uint8_t value = (packed >> (8 - bpp * (i + 1))) & ((1 << bpp) - 1);

                    if (bpp == 1)
                        row[px] = value ? 15 : 0;
                    else
                        row[px] = (uint8_t[]){0, 8, 7, 15}[value];
                }
            }
        } else {
            uint32_t px = 0;

            while (px < width) {
                if (!run_count) {
                    if (pos + 2 > size)
                        return 1;

                    run_count = data[pos++];
                    run_value = data[pos++];

                    if (!run_count)
                        return 1;
                }

                uint32_t n = run_count;

                if (n > width - px)
                    n = width - px;

                for (uint32_t i = 0; i < n; i++) {
                    if (run_value >= 16)
                        return 1;

                    row[px++] = run_value;
                }

                run_count -= n;
            }
        }

        int dy0 = (int)(sy * visual_size);
        int dy1 = (int)((sy + 1) * visual_size);

        for (int dy = dy0; dy < dy1; dy++) {
            for (uint32_t px = 0; px < width; px++) {
                int dx0 = (int)(px * visual_size);
                int dx1 = (int)((px + 1) * visual_size);

                for (int dx = dx0; dx < dx1; dx++)
                    putpixel(x + dx, y + dy, row[px]);
            }
        }
    }

    return 0;
}
