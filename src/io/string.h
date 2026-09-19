#pragma once

#include <stdint.h>

int strcmp(const char* a, const char* b);
void *memset(void *dest, int value, uint32_t size);
void *memcpy(void *dest, const void *src, unsigned int n);
void strcpy(char *dest, char *src, uint32_t count);
unsigned long strlen(char *s);
int split(char *s, char ret[32][128]);
void itoa(uint32_t n, char *buf);
int parse_int(char *s);
void strappend(char *dest, const char *src);
