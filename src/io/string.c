#include "io/string.h"
#include <stdint.h>
int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

void *memset(void *dest, int value, uint32_t size) {
    uint8_t *p = dest;

    for (uint32_t i = 0; i < size; i++)
        p[i] = (uint8_t)value;

    return dest;
}

void *memcpy(void *dest, const void *src, unsigned int n) {
    unsigned char *d = dest;
    const unsigned char *s = src;

    for (unsigned int i = 0; i < n; i++)
        d[i] = s[i];

    return dest;
}

void strcpy(char *dest, char *src, uint32_t count) {
    for (uint32_t i = 0; i < count - 1 && src[i]; i++)
        dest[i] = src[i];

    dest[count - 1] = '\0';
}

unsigned long strlen(char *s) {
    unsigned long i = 0;
    while (*s++) i++;
    return i;
}

int split(char *s, char ret[32][128]) {
    int pos = 0;
    int argc = 0;

    while (s[pos] && argc < 32) {
        int current_pos = 0;

        // Skip spaces
        while (s[pos] == ' ')
            pos++;

        if (!s[pos])
            break;

        // Copy one argument
        while (s[pos] && s[pos] != ' ' && current_pos < 127) {
            ret[argc][current_pos] = s[pos];
            current_pos++;
            pos++;
        }

        ret[argc][current_pos] = '\0';
        argc++;
    }

    return argc;
}

void itoa(uint32_t n, char *buf) {
    char tmp[11];
    int i = 0;
    int j = 0;

    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (n > 0) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }

    while (i > 0)
        buf[j++] = tmp[--i];

    buf[j] = '\0';
}

int parse_int(char *s) {
    int n = 0;

    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');

    return n;
}

void strappend(char *dest, const char *src) {
    while (*dest) dest++;

    while (*src)
        *dest++ = *src++;

    *dest = '\0';
}
