#pragma once

#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define SHM_NAME "/00753499_arc-set_myshm"
#define SEM_FREE_NAME "/00753499_arc-set_free"
#define SEM_USED_NAME "/00753499_arc-set_used"
#define SEM_WRITE_NAME "/00753499_arc-set_write"

typedef struct Edge {
  int a, b;
} Edge_t;

#define MAX_REPORTED 8
typedef struct myvect {
  size_t size;
  Edge_t arc_set[MAX_REPORTED];
} Result_t;

#define BUF_LEN 64
typedef struct myshm {
  Result_t buf[BUF_LEN];
  int read_pos;
  int write_pos;
  bool shutdown;
} Myshm_t;
