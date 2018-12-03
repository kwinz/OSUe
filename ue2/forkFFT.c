#include "tools.h"
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT_LINEBUFFER_SIZE 1
#define INITIAL_ARRAY_CAPACITY 2

typedef struct myvect {
  float *data;
  size_t size;
  size_t capacity;
} Myvect_t;

static void init_myvect(Myvect_t *myvect) {
  myvect->data = malloc(sizeof(float) * INITIAL_ARRAY_CAPACITY);
  myvect->size = 0;
  myvect->capacity = INITIAL_ARRAY_CAPACITY;
}

static void push_myvect(Myvect_t *myvect, float data) {
  if (unlikely(myvect->size == myvect->capacity)) {
    myvect->capacity *= 2;
    myvect->data = realloc(myvect->data, sizeof(float) * myvect->capacity);
  }
  myvect->data[myvect->size] = data;
  myvect->size++;
}

static void freedata_myvect(Myvect_t *myvect) {
  free(myvect->data);
  myvect->size = 0;
  myvect->capacity = 0;
}

int main(int argc, char *argv[]) {

  size_t linebufferSize = DEFAULT_LINEBUFFER_SIZE;
  char *line = malloc(linebufferSize);

  Myvect_t myVect;
  init_myvect(&myVect);

  int linecount = 0;
  while ((getline(&line, &linebufferSize, stdin)) != -1) {
    ++linecount;

    char *endPointer;
    const float valueOfThisLine = strtof(&line, &endPointer);

    push_myvect(&myVect, valueOfThisLine);
  }

  if (myVect.size == 1) {
    fprintf(stdout, "%f", myVect.data[myVect.size]);
  }

  if (myVect.size % 2 != 0) {
    return EXIT_FAILURE;
  }

  int pipefdEven[2];
  {
    pipe(pipefdEven);
    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
      // we are a child

      // close unused write end
      lose(pipefdEven[1]);

      // old descriptor - read end to new descriptor
      dup2(pipefdEven[0], STDIN_FILENO);

      close(pipefdEven[0]);

      execlp(argv[0], argv[0], "", NULL);
    } else {
      // we are a parent
    }
  }

  int pipefdOdd[2];
  {
    pipe(pipefdOdd);
    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
      // we are a child

      // close unused write end
      lose(pipefdOdd[1]);

      // old descriptor - read end to new descriptor
      dup2(pipefdOdd[0], STDIN_FILENO);

      close(pipefdOdd[0]);

      execlp(argv[0], argv[0], "", NULL);
    } else {
      // we are a parent
    }
  }

  return EXIT_SUCCESS;
}