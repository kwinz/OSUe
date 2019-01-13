#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tools.h"

/** @defgroup supervisor */

/** @addtogroup supervisor
 * @brief Supervises generators of feedback arc sets.
 *
 * @details Sets up shared memory with a ringbuffer and semaphores and accepts feedback arc sets
 * from the generator programm. Memorizes the smallest (best) solutions generated.
 *
 * @author Markus Krainz
 * @date January 2019
 * @{
 */

static char *program_name;

static int shmfd;
static Myshm_t *myshm;
static sem_t *free_sem, *used_sem, *write_sem;

static volatile sig_atomic_t quit = 0;

/**
 * @brief Reacts to SIGINT and SIGTERM by setting quit to 1
 */
static void handle_signal(int signal) { quit = 1; }

/**
 * @brief Reads a new feedback arc set
 *
 * @details Reads the arc set result to the ringbuffer in shm, using the semaphores passed.
 * Blocks until the buffer contains a set.
 *
 * @param shm Pointer to shared memory
 * @param free_sem Pointer to the semaphore guarding the free space
 * @param used_sem Pointer to the semaphore guarding the used space
 * @param best_result if the new set is better than the set which is referenced by best_result,
 * best_result will be updated with the new set.
 * @return true if best_result has been updated, false otherwise
 */
static bool circ_buf_read(Myshm_t *shm, sem_t *free_sem, sem_t *used_sem, Result_t *best_result) {
  bool newBest = false;

  // reading requires data (used space)
  if (sem_wait(used_sem) == -1) {
    if (errno == EINTR) {
      // harmless interrupt. Return now best_result and continue
      return false;
    }
    fprintf(stderr, "%s %d: ERROR with sem_wait.\n", program_name, (int)getpid());
    exit(EXIT_FAILURE);
  }

  const Result_t *result = &(shm->buf[shm->read_pos]);
  if (result->size < best_result->size) {
    memcpy(best_result, result, sizeof(Result_t));
    newBest = true;
  }
  // reading frees up space
  if (sem_post(free_sem) == -1) {
    fprintf(stderr, "%s %d: ERROR with sem_post.\n", program_name, (int)getpid());
    exit(EXIT_FAILURE);
  }
  shm->read_pos = (shm->read_pos + 1) % BUF_LEN;

  if (DEBUG_OUTPUT) {
    fprintf(stderr, ".");
  }
  return newBest;
}

/**
 * @brief Frees any resource of the programm, that might have been opened.
 *
 * @details Unmaps, frees, deletes resources atexit and writes error messages to stderr.
 */
static void free_resources(void) {
  if (munmap(myshm, sizeof(Myshm_t)) == -1) {
    fprintf(stderr, "%s %d: Could not unmap shm\n", program_name, (int)getpid());
  }

  if (shmfd != -1) {
    if (close(shmfd) == -1) {
      fprintf(stderr, "%s %d: Could not close shm file\n", program_name, (int)getpid());
    }
  }

  // remove shared memory object:
  if (shm_unlink(SHM_NAME) == -1) {
    fprintf(stderr, "%s %d: Could not unlink (remove) shm file\n", program_name, (int)getpid());
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

  if (sem_unlink(SEM_FREE_NAME) == -1) {
    fprintf(stderr, "%s %d: Could not unlink (delete) free_sem.\n", program_name, (int)getpid());
  }
  if (sem_unlink(SEM_USED_NAME) == -1) {
    fprintf(stderr, "%s %d: Could not unlink (delete) used_sem.\n", program_name, (int)getpid());
  }
  if (sem_unlink(SEM_WRITE_NAME) == -1) {
    fprintf(stderr, "%s %d: Could not unlink (delete) write_sem.\n", program_name, (int)getpid());
  }

  fprintf(stderr, "%s %d: Finished atexit() handler \n", program_name, (int)getpid());
}

int main(int argc, char *argv[]) {
  program_name = argv[0];

  if (atexit(free_resources) != 0) {
    fprintf(stderr, "%s %d: ERROR Could register atexit handler.\n", program_name, (int)getpid());
    return EXIT_FAILURE;
  }

  // create and/or open the shared memory object:
  shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
  if (shmfd == -1) {
    fprintf(stderr, "%s %d: ERROR Could not open shm file\n", program_name, (int)getpid());
    return EXIT_FAILURE;
  }
  // set the size of the shared memory:
  if (ftruncate(shmfd, sizeof(Myshm_t)) < 0) {
    fprintf(stderr, "%s %d: ERROR Could not truncate (resize) shm file\n", program_name,
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
  free_sem = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, BUF_LEN);
  if (free_sem == SEM_FAILED) {
    fprintf(stderr, "%s %d: ERROR Could not open SEM_FREE_NAME\n", program_name, (int)getpid());
    printf("%s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  // tracks used space, initialized to 0
  used_sem = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0);
  if (used_sem == SEM_FAILED) {
    fprintf(stderr, "%s %d: ERROR Could not open SEM_USED_NAME\n", program_name, (int)getpid());
    printf("%s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  // assures at most 1 writer
  write_sem = sem_open(SEM_WRITE_NAME, O_CREAT | O_EXCL, 0600, 1);
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

  Result_t best_result;
  best_result.size = MAX_REPORTED + 1;

  do {
    // blocking read
    const bool newBest = circ_buf_read(myshm, free_sem, used_sem, &best_result);

    if (newBest) {
      fprintf(stdout, "Solution with %zu edges: ", best_result.size);
      for (int i = 0; i < best_result.size; ++i) {
        fprintf(stdout, "%d-%d ", best_result.arc_set[i].a, best_result.arc_set[i].b);
      }
      fprintf(stdout, "\n");
    }

  } while (!quit && best_result.size != 0);

  fprintf(stderr, "Finished processing!\n");

  myshm->shutdown = true;

  return EXIT_SUCCESS;
}

/** @}*/