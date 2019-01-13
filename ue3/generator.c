#include <assert.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tools.h"

static volatile sig_atomic_t quit = 0;

static void handle_signal(int signal) { quit = 1; }

// taken heavy inspiration from "Exercise 3: Shared Memory [..]" slides Platzer (2018)
static void circ_buf_write(Myshm_t *shm, sem_t *free_sem, sem_t *used_sem, sem_t *write_sem,
                           Result_t *val) {
  // writing requires free space
  sem_wait(free_sem);
  shm->buf[shm->write_pos] = *val;
  // space is used by written data
  sem_post(used_sem);
  shm->write_pos = (shm->write_pos + 1) % BUF_LEN;
}

int main(int argc, char *argv[]) {

  const int edge_count = argc - 1;
  fprintf(stderr, "edges: %d\n", edge_count);

  if (argc == 1) {
    fprintf(stderr, "Provide at least one edge. \n");
    return EXIT_FAILURE;
  }

  // C99 VLA
  Edge_t graph[edge_count];

  // parse edge_count from arguments
  int max_vert = 0;
  for (int i = 0; i < edge_count; i++) {
    char *a_str = strtok(argv[i + 1], "-");
    if (a_str == NULL) {
      fprintf(stderr, "Invalid edge, no part a (did you pass \"\"?) \n");
      return EXIT_FAILURE;
    }
    char *b_str = strtok(NULL, "-");
    if (b_str == NULL) {
      fprintf(stderr, "Invalid edge, no part b \n");
      return EXIT_FAILURE;
    }

    char *endPointer;
    long a = strtol(a_str, &endPointer, 0);
    if (*endPointer != '\0') {
      fprintf(stderr, "Could not parse edge part a\n");
      return EXIT_FAILURE;
    }

    long b = strtol(b_str, &endPointer, 0);
    if (*endPointer != '\0') {
      fprintf(stderr, "Could not parse edge part b\n");
      return EXIT_FAILURE;
    }

    graph[i].a = a;
    graph[i].b = b;

    max_vert = fmax(a, max_vert);
    max_vert = fmax(b, max_vert);
  }

  fprintf(stderr, "vertices: %d\n", max_vert + 1);

  // create array of vertices
  int vertices[max_vert + 1];
  for (int i = 0; i <= max_vert; ++i) {
    vertices[i] = i;
  }

  Result_t report;
  srand(time(NULL));

  // open the shared memory object:
  int shmfd = shm_open(SHM_NAME, O_RDWR, 0600);
  if (shmfd == -1) {
    fprintf(stderr, "ERROR No shm. (Start supervisor first!)\n");
    return EXIT_FAILURE;
  }

  // map shared memory object:
  Myshm_t *myshm;
  myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

  if (myshm == MAP_FAILED) {
    fprintf(stderr, "Could not memory map shm file\n");
    return EXIT_FAILURE;
  }

  // tracks free space, initialized to BUF_LEN
  sem_t *free_sem = sem_open(SEM_FREE_NAME, /*flags*/ 0);
  // tracks used space, initialized to 0
  sem_t *used_sem = sem_open(SEM_USED_NAME, 0);
  // assures at most 1 writer
  sem_t *write_sem = sem_open(SEM_WRITE_NAME, 0);

  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // initialize sa to 0
    sa.sa_handler = &handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
  }

  do {
    // shuffle
    for (int i = 0; i < max_vert - 1; ++i) {
      const int j = i + rand() % (max_vert + 1 - i);
      const int temp = vertices[i];
      vertices[i] = vertices[j];
      vertices[j] = temp;
    }

    if (DEBUG_OUTPUT) {
      fprintf(stderr, "shuffled: ");
      for (int i = 0; i <= max_vert; ++i) {
        fprintf(stderr, "%d,", vertices[i]);
      }
      fprintf(stderr, "\n");
    }

    bool max_exceeded = false;
    report.size = 0;

    for (int i = 0, reported = 0; i < edge_count && !max_exceeded; ++i) {
      for (int j = 0; j <= max_vert; ++j) {
        if (graph[i].a == vertices[j]) {
          if (DEBUG_OUTPUT) {
            fprintf(stderr, "-[%d,%d]\n", graph[i].a, graph[i].b);
          }
          break;
        }
        if (graph[i].b == vertices[j]) {
          if (reported == MAX_REPORTED) {
            max_exceeded = true;
            break;
          }
          if (DEBUG_OUTPUT) {
            fprintf(stderr, "+[%d,%d]\n", graph[i].a, graph[i].b);
          }
          report.arc_set[reported] = graph[i];
          ++reported;
          ++report.size;
          break;
        }
        assert(j < max_vert && "No matches found yet. This should never happen.");
      }
    }

    if (DEBUG_OUTPUT) {
      fprintf(stderr, "exceeded:%s\n", max_exceeded ? "true" : "false");
    }

    if (myshm->shutdown) {
      printf("shutting down beacuse of shm signal: %s\n", argv[0]);
      quit = true;
      break;
    }

    if (!max_exceeded) {

      sem_wait(write_sem);
      circ_buf_write(myshm, free_sem, used_sem, write_sem, &report);

      // we could sleep here for 500ms with usleep(500000); e.g. for debugging
      sem_post(write_sem);
    }

  } while (!quit);

  if (munmap(myshm, sizeof(*myshm)) == -1) {
    fprintf(stderr, "Could not unmap shm\n");
    return EXIT_FAILURE;
  }

  if (close(shmfd) == -1) {
    fprintf(stderr, "Could not close shm file\n");
    return EXIT_FAILURE;
  }

  if (sem_close(free_sem) == -1) {
    fprintf(stderr, "Could not close free_sem.\n");
    return EXIT_FAILURE;
  }
  if (sem_close(used_sem) == -1) {
    fprintf(stderr, "Could not close used_sem.\n");
    return EXIT_FAILURE;
  }
  if (sem_close(write_sem) == -1) {
    fprintf(stderr, "Could not close write_sem.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}