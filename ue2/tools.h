#pragma once

#include <stdio.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define PI 3.141592654

typedef struct myvect {
  float *data;
  size_t size;
  size_t capacity;
} Myvect_t;

#define INITIAL_ARRAY_CAPACITY 2
void init_myvect(Myvect_t *myvect);
void push_myvect(Myvect_t *myvect, float data);
void freedata_myvect(Myvect_t *myvect);