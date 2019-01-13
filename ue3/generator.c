#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tools.h"

/** @defgroup generator */

/** @addtogroup generator
 * @brief Generates Feedback arc set
 *
 * @details Parses a directed graph from command line. Then uses a random algorithm to find
 * feedback arc sets. Then the arc sets smaller than MAX_REPORTED are shared with the supervisor
 * programm. Taken heavy inspiration from "Exercise 3: Shared Memory [..]" slides Platzer (2018)
 *
 * @author Markus Krainz
 * @date January 2019
 * @{
 */

static char *program_name;

static volatile sig_atomic_t quit = 0;

static int shmfd;
static Myshm_t *myshm;
static sem_t *free_sem, *used_sem, *write_sem;

/**
 * @brief Reacts to SIGINT and SIGTERM by setting quit to 1
 */
static void handle_signal(int signal) { quit = 1; }

/**
 * @brief Writes a new feedback arc set
 *
 * @details Writes the arc set result to the ringbuffer in shm, wusing the semaphores passed.
 * Blocks until the buffer is free.
 *
 * @param shm Pointer to shared memory
 * @param free_sem Pointer to the semaphore guarding the free space
 * @param used_sem Pointer to the semaphore guarding the used space
 * @param val points to the Result_t that should be written into shared memory
 */
static void circ_buf_write(Myshm_t *shm, sem_t *free_sem, sem_t *used_sem, Result_t *val) {
  // writing requires free space
  if (sem_wait(free_sem) == -1) {
    if (errno == EINTR) {
      // harmless interrupt. Throw away current result and continue.
      return;
    }
    fprintf(stderr, "%s %d: ERROR with sem_wait.\n", program_name, (int)getpid());
    exit(EXIT_FAILURE);
  }
  shm->buf[shm->write_pos] = *val;
  // space is used by written data
  if (sem_post(used_sem) == -1) {
    fprintf(stderr, "%s %d: ERROR with sem_post.\n", program_name, (int)getpid());
    exit(EXIT_FAILURE);
  }
  shm->write_pos = (shm->write_pos + 1) % BUF_LEN;
}

static void free_resources(void) {
  if (munmap(myshm, sizeof(Myshm_t)) == -1) {
    fprintf(stderr, "%s %d: Could not unmap shm\n", program_name, (int)getpid());
  }

  if (shmfd != -1) {
    if (close(shmfd) == -1) {
      fprintf(stderr, "%s %d: Could not close shm file\n", program_name, (int)getpid());
    }
  }

  if (sem_close(free_sem) == -1) {
    fprintf(stderr, "%s %d: Could not close free_sem.\n", program_name, (int)getpid());
  }
  if (sem_close(used_sem) == -1) {
    fprintf(stderr, "%s %d: Could not close used_sem.\n", program_name, (int)getpid());
  }
  if (sem_close(write_sem) == -1) {
    fprintf(stderr, "%s %d: Could not close write_sem.\n", program_name, (int)getpid());
  }

  fprintf(stderr, "%s %d: Finished atexit() handler \n", program_name, (int)getpid());
}

int main(int argc, char *argv[]) {
  program_name = argv[0];

  const int edge_count = argc - 1;
  fprintf(stderr, "edges: %d\n", edge_count);

  if (argc == 1) {
    fprintf(stderr, "%s %d: ERROR Provide at least one edge. \n", program_name, (int)getpid());
    return EXIT_FAILURE;
  }

  if (atexit(free_resources) != 0) {
    fprintf(stderr, "%s %d: ERROR Could register atexit handler.\n", program_name, (int)getpid());
    return EXIT_FAILURE;
  }

  // C99 VLA
  Edge_t graph[edge_count];

  // parse edge_count from arguments
  int max_vert = 0;
  for (int i = 0; i < edge_count; i++) {
    char *a_str = strtok(argv[i + 1], "-");
    if (a_str == NULL) {
      fprintf(stderr, "%s %d: ERROR Invalid edge, no part a (did you pass \"\"?) \n", program_name,
              (int)getpid());
      return EXIT_FAILURE;
    }
    char *b_str = strtok(NULL, "-");
    if (b_str == NULL) {
      fprintf(stderr, "%s %d: ERROR Invalid edge, no part b \n", program_name, (int)getpid());
      return EXIT_FAILURE;
    }

    char *endPointer;
    long a = strtol(a_str, &endPointer, 0);
    if (*endPointer != '\0') {
      fprintf(stderr, "%s %d: ERROR Could not parse edge part a\n", program_name, (int)getpid());
      return EXIT_FAILURE;
    }

    long b = strtol(b_str, &endPointer, 0);
    if (*endPointer != '\0') {
      fprintf(stderr, "%s %d: ERROR Could not parse edge part b\n", program_name, (int)getpid());
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
  shmfd = shm_open(SHM_NAME, O_RDWR, 0600);
  if (shmfd == -1) {
    fprintf(stderr, "%s %d: ERROR No shm. (Start supervisor first!)\n", program_name,
            (int)getpid());
    return EXIT_FAILURE;
  }

  // map shared memory object:
  myshm = mmap(NULL, sizeof(Myshm_t), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (myshm == MAP_FAILED) {
    fprintf(stderr, "%s %d: ERROR Could not memory map shm file\n", program_name, (int)getpid());
    return EXIT_FAILURE;
  }

  // tracks free space, initialized to BUF_LEN
  free_sem = sem_open(SEM_FREE_NAME, /*flags*/ 0);
  if (free_sem == SEM_FAILED) {
    fprintf(stderr, "%s %d: ERROR Could not open SEM_FREE_NAME\n", program_name, (int)getpid());
    printf("%s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  // tracks used space, initialized to 0
  used_sem = sem_open(SEM_USED_NAME, 0);
  if (used_sem == SEM_FAILED) {
    fprintf(stderr, "%s %d: ERROR Could not open SEM_USED_NAME\n", program_name, (int)getpid());
    printf("%s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  // assures at most 1 writer
  write_sem = sem_open(SEM_WRITE_NAME, 0);
  if (write_sem == SEM_FAILED) {
    fprintf(stderr, "%s %d: ERROR Could not open SEM_WRITE_NAME\n", program_name, (int)getpid());
    printf("%s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  // setup signal handlers
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
      printf("%s %d: shutting down because of shm signal.\n", program_name, (int)getpid());
      quit = true;
      break;
    }

    if (!max_exceeded) {
      if (sem_wait(write_sem) == -1) {
        if (errno == EINTR) {
          // harmless interrupt. Throw away current result and continue.
          continue;
        }
        fprintf(stderr, "%s %d: ERROR with sem_wait.\n", program_name, (int)getpid());
        return (EXIT_FAILURE);
      }

      circ_buf_write(myshm, free_sem, used_sem, &report);

      if (sem_post(write_sem) == -1) {
        fprintf(stderr, "%s %d: ERROR with sem_post.\n", program_name, (int)getpid());
        return (EXIT_FAILURE);
      }
      // we could sleep here for 500ms with usleep(500000); e.g. for debugging
    }

  } while (!quit);

  return EXIT_SUCCESS;
}

/** @}*/
