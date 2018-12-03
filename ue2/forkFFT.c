#include "tools.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
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

  if (myVect.size == 0) {
    return EXIT_FAILURE;
  }

  if (myVect.size == 1) {
    fprintf(stdout, "%f", myVect.data[myVect.size]);
    return EXIT_SUCCESS;
  }

  if (myVect.size % 2 != 0) {
    return EXIT_FAILURE;
  }

  int pipefdEven[2];
  pid_t pidEven;
  {
    pipe(pipefdEven);
    fflush(stdout);
    pidEven = fork();

    if (pidEven == 0) {
      // we are a child

      // close unused write end
      // close(pipefdEven[1]);

      dup2(pipefdEven[1], STDOUT_FILENO);
      close(pipefdEven[1]);

      // old descriptor - read end to new descriptor
      dup2(pipefdEven[0], STDIN_FILENO);
      close(pipefdEven[0]);

      execlp(argv[0], argv[0], "", NULL);
    } else {
      // we are a parent
    }
  }

  int pipefdOdd[2];
  pid_t pidOdd;
  {
    pipe(pipefdOdd);
    fflush(stdout);
    pid_t pidOdd = fork();

    if (pidOdd == 0) {
      // we are a child

      // close unused write end
      // close(pipefdOdd[1]);
      dup2(pipefdEven[1], STDOUT_FILENO);
      close(pipefdEven[1]);

      // old descriptor - read end to new descriptor
      dup2(pipefdOdd[0], STDIN_FILENO);
      close(pipefdOdd[0]);

      execlp(argv[0], argv[0], "", NULL);
    } else {
      // we are a parent
    }
  }

  // send data to children
  for (size_t i = 0; i < myVect.size; i += 2) {
    fprintf(pipefdEven[0], "%f", myVect.data[i]);
  }
  for (size_t i = 1; i < myVect.size; i += 2) {
    fprintf(pipefdOdd[0], "%f", myVect.data[i]);
  }

  fputc(EOF, pipefdEven[0]);
  fputc(EOF, pipefdOdd[0]);

  const size_t resultSize = myVect.size;
  freedata_myvect(&myVect);

  // C99 Variable Length Array
  // float results[resultSize];

  size_t k = 0;
  for (; k < resultSize; ++k) {
    float result;
    char *endPointer;

    int res = getline(&line, &linebufferSize, pipefdEven[1]);
    if (res == -1) {
      // error
    }
    const float valueOfEven = strtof(&line, &endPointer);
    result = valueOfEven;

    res = getline(&line, &linebufferSize, pipefdEven[1]);
    if (res == -1) {
      // error
    }
    const float valueOfOdd = strtof(&line, &endPointer);
    result += valueOfOdd;

    fprintf(stdout, "%f", result);
  }

  // wait for children to die
  bool evenDead = false, oddDead = false;
  while (true) {
    int status;
    pid_t pid = wait(&status);

    if (pid == -1) {
      // an error occured
      if (errno == EINTR) {
        // EINTR is harmless retry
        continue;
      }
      // else crash
      fprintf(stderr, "Cannot wait!\n");
      exit(EXIT_FAILURE);
    }

    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }

    if (pid == pidOdd) {
      oddDead = true;
    } else if (pid == pidEven) {
      evenDead = true;
    }

    if (oddDead && evenDead) {
      break;
    }
  }

  return EXIT_SUCCESS;
}
