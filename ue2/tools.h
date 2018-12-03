#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

int8_t startsWith(const char *longstring, const char *begin);
int8_t strEndsWith(char *str, char *suffix);
void printVerbose(int verbose, const char *fmt, va_list args);

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
