#include <stdint.h>
#include <stdarg.h>
#include "io/string.h"
#include "logic/terminal.h"
#include "kernel/printk.h"

static void printk_append(char *buf, uint32_t *pos, const char *str) {
    while (*str && *pos < 1023)
        buf[(*pos)++] = *str++;
}

static void printk_hex(uint32_t n, char *buf, uint32_t *pos) {
    char hex[] = "0123456789ABCDEF";

    if (*pos > 1015)
        return;

    for (int i = 7; i >= 0; i--) {
        buf[*pos + i] = hex[n & 0xF];
        n >>= 4;
    }

    *pos += 8;
}

void printk(const char *fmt, ...) {
    char buf[1024];
    uint32_t pos = 0;
    va_list args;

    va_start(args, fmt);

    while (*fmt && pos < 1023) {
        if (*fmt != '%') {
            buf[pos++] = *fmt++;
            continue;
        }

        fmt++;

        switch (*fmt) {
            case '%':
                buf[pos++] = '%';
                break;

            case 's':
                printk_append(buf, &pos, va_arg(args, char *));
                break;

            case 'c':
                buf[pos++] = (char)va_arg(args, int);
                break;

            case 'u': {
                char tmp[11];
                itoa(va_arg(args, uint32_t), tmp);
                printk_append(buf, &pos, tmp);
                break;
            }

            case 'x':
                printk_hex(va_arg(args, uint32_t), buf, &pos);
                break;

            default:
                if (pos < 1021) {
                    buf[pos++] = '%';
                    buf[pos++] = *fmt;
                }
                break;
        }

        fmt++;
    }

    va_end(args);

    buf[pos] = '\0';
    puts(buf);
    terminal_draw();
}
