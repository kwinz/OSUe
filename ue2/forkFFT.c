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

  fprintf(stderr, "Running main()... My pid is: %d\n", (int)getpid());

  size_t linebufferSize = DEFAULT_LINEBUFFER_SIZE;
  char *line = malloc(linebufferSize);

  Myvect_t myVect;
  init_myvect(&myVect);

  int linecount = 0;
  fprintf(stderr, "Waiting for a line.... My pid is: %d\n", (int)getpid());

  while ((getline(&line, &linebufferSize, stdin)) != -1) {
    ++linecount;
    fprintf(stderr, "Got a line. My pid is: %d\n", (int)getpid());

    char *endPointer;
    const float valueOfThisLine = strtof(line, &endPointer);

    push_myvect(&myVect, valueOfThisLine);
  }

  fprintf(stderr, "Read %zu floats from stdin. My pid is: %d\n", myVect.size, (int)getpid());

  if (myVect.size == 0) {
    return EXIT_FAILURE;
  }

  if (myVect.size == 1) {
    fprintf(stdout, "%f", myVect.data[0]);
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

      execlp(argv[0], argv[0], NULL);
    } else {
      // we are a parent
    }
  }

  int pipefdOdd[2];
  pid_t pidOdd = 0;
  {
    pipe(pipefdOdd);
    fflush(stdout);
    pid_t pidOdd = fork();

    if (pidOdd == 0) {
      // we are a child

      // close unused write end
      // close(pipefdOdd[1]);
      dup2(pipefdOdd[1], STDOUT_FILENO);
      close(pipefdOdd[1]);

      // old descriptor - read end to new descriptor
      dup2(pipefdOdd[0], STDIN_FILENO);
      close(pipefdOdd[0]);

      execlp(argv[0], argv[0], "", NULL);
    } else {
      // we are a parent
    }
  }

  fprintf(stderr, "Send data to children...\n");
  // send data to children
  {
    FILE *childEvenFp = fdopen(pipefdEven[1], "w");
    FILE *childOddFp = fdopen(pipefdOdd[1], "w");

    for (size_t i = 0; i < myVect.size; i += 2) {
      fprintf(childEvenFp, "%f\n", myVect.data[i]);
    }
    for (size_t i = 1; i < myVect.size; i += 2) {
      fprintf(childOddFp, "%f\n", myVect.data[i]);
    }

    fputc(EOF, childEvenFp);
    fputc(EOF, childOddFp);

    fflush(childEvenFp);
    fflush(childOddFp);

    // int lol = myVect.data[i];
    // fprintf(stderr, "LOOOOOOOOOLLLL %d", pipefdEven[0]);

    fclose(childEvenFp);
    fclose(childOddFp);
    close(pipefdEven[1]);
    close(pipefdOdd[1]);
  }

  const size_t resultSize = myVect.size;
  freedata_myvect(&myVect);

  // C99 Variable Length Array
  // float results[resultSize];

  fprintf(stderr, "Read data from children...\n");
  // read data from children
  {
    FILE *childEvenFp = fdopen(pipefdEven[0], "r");
    FILE *childOddFp = fdopen(pipefdOdd[0], "r");

    size_t k = 0;
    for (; k < resultSize; ++k) {
      float result;
      char *endPointer;

      int res = getline(&line, &linebufferSize, childEvenFp);
      if (res == -1) {
        exit(EXIT_FAILURE);
      }
      const float valueOfEven = strtof(line, &endPointer);
      result = valueOfEven;

      res = getline(&line, &linebufferSize, childOddFp);
      if (res == -1) {
        exit(EXIT_FAILURE);
      }
      const float valueOfOdd = strtof(line, &endPointer);
      result += valueOfOdd;

      fprintf(stdout, "%f", result);
    }
  }

  fprintf(stderr, "Wait for children to die...\n");
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
