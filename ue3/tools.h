#pragma once

#include <stdio.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

typedef struct Edge {
  int a, b;
} Edge_t;

#define MAX_REPORTED 4

typedef struct myvect {
  size_t size;
  Edge_t arc_set[MAX_REPORTED];
} Result_t;


#define INITIAL_ARRAY_CAPACITY 2
