#include "tools.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <math.h>

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
  // fprintf(stderr, "Waiting for a line.... My pid is: %d\n", (int)getpid());

  while ((getline(&line, &linebufferSize, stdin)) != -1) {
    ++linecount;
    // fprintf(stderr, "Got a line. My pid is: %d\n", (int)getpid());

    char *endPointer;
    const float valueOfThisLine = strtof(line, &endPointer);

    push_myvect(&myVect, valueOfThisLine);
  }

  fprintf(stderr, "Read %zu floats from stdin. My pid is: %d\n", myVect.size, (int)getpid());

  if (myVect.size == 0) {
    return EXIT_FAILURE;
  }

  if (myVect.size == 1) {
    fprintf(stdout, "%f 0.0*i", myVect.data[0]);
    fprintf(stderr, "Wrote result! My pid is: %d\n", (int)getpid());
    return EXIT_SUCCESS;
  }

  if (myVect.size % 2 != 0) {
    return EXIT_FAILURE;
  }

  int evenStdin;
  int evenStdout;
  pid_t pidEven;
  {
    int pipePairStdin[2];
    int pipePairStdout[2];
    pipe(pipePairStdin);
    pipe(pipePairStdout);

    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
      // we are a child

      // pipefd[0] – read end
      // pipefd[1] – write end

      dup2(pipePairStdout[1], STDOUT_FILENO);
      close(pipePairStdout[1]);
      close(pipePairStdout[0]);

      // old descriptor - read end to new descriptor
      dup2(pipePairStdin[0], STDIN_FILENO);
      close(pipePairStdin[0]);
      close(pipePairStdin[1]);

      execlp(argv[0], argv[0], NULL);
      // execlp only returns if an error has occured
      exit(EXIT_FAILURE);
    } else if (pid == -1) {
      // an error has occured
      exit(EXIT_FAILURE);
    }
    // we are a parent
    evenStdin = pipePairStdin[1];
    close(pipePairStdin[0]);
    evenStdout = pipePairStdout[0];
    close(pipePairStdout[1]);
    pidEven = pid;
  }

  int oddStdin;
  int oddStdout;
  pid_t pidOdd;
  {
    int pipePairStdin[2];
    int pipePairStdout[2];
    pipe(pipePairStdin);
    pipe(pipePairStdout);

    fflush(stdout);
    pid_t pid = fork();

    if (pid == 0) {
      // we are a child

      // pipefd[0] – read end
      // pipefd[1] – write end

      dup2(pipePairStdout[1], STDOUT_FILENO);
      close(pipePairStdout[1]);
      close(pipePairStdout[0]);

      // old descriptor - read end to new descriptor
      dup2(pipePairStdin[0], STDIN_FILENO);
      close(pipePairStdin[0]);
      close(pipePairStdin[1]);

      execlp(argv[0], argv[0], NULL);
      // execlp only returns if an error has occured
      exit(EXIT_FAILURE);
    } else if (pid == -1) {
      // an error has occured
      exit(EXIT_FAILURE);
    }
    // we are a parent
    oddStdin = pipePairStdin[1];
    close(pipePairStdin[0]);
    oddStdout = pipePairStdout[0];
    close(pipePairStdout[1]);
    pidOdd = pid;
  }

  fprintf(stderr, "Send data to children...\n");
  // send data to children
  {
    FILE *childEvenFp = fdopen(evenStdin, "w");
    FILE *childOddFp = fdopen(oddStdin, "w");

    for (size_t i = 0; i < myVect.size; i += 2) {
      fprintf(childEvenFp, "%f\n", myVect.data[i]);
    }
    for (size_t i = 1; i < myVect.size; i += 2) {
      fprintf(childOddFp, "%f\n", myVect.data[i]);
    }

    // fputc(EOF, childEvenFp);
    // fputc(EOF, childOddFp);

    // fflush(childEvenFp);
    // fflush(childOddFp);

    // int lol = myVect.data[i];
    // fprintf(stderr, "LOOOOOOOOOLLLL %d", pipefdEven[0]);

    fclose(childEvenFp);
    fclose(childOddFp);
    close(evenStdin);
    close(oddStdin);
  }

  const size_t resultSize = myVect.size;
  freedata_myvect(&myVect);

  fprintf(stderr, "Read data from children...\n");
  // read data from children
  // C99 Variable Length Arrays
  float resultEven[resultSize / 2];
  float resultOdd[resultSize / 2];
  float resultEvenImaginary[resultSize / 2];
  float resultOddImaginary[resultSize / 2];
  {
    FILE *childEvenFp = fdopen(evenStdout, "r");
    FILE *childOddFp = fdopen(oddStdout, "r");

    for (size_t k = 0; k < resultSize / 2; ++k) {
      char *endPointer;

      int res = getline(&line, &linebufferSize, childEvenFp);
      if (res == -1) {
        fprintf(stderr, "Cannot read even! My pid is: %d\n", (int)getpid());
        exit(EXIT_FAILURE);
      }
      resultEven[k] = strtof(line, &endPointer);
      resultEvenImaginary[k] = strtof(endPointer, &endPointer);

      res = getline(&line, &linebufferSize, childOddFp);
      if (res == -1) {
        fprintf(stderr, "Cannot read odd! My pid is: %d\n", (int)getpid());
        exit(EXIT_FAILURE);
      }
      resultOdd[k] = strtof(line, &endPointer);
      resultOddImaginary[k] = strtof(endPointer, &endPointer);
    }

    fclose(childEvenFp);
    fclose(childOddFp);
    close(evenStdout);
    close(oddStdout);
  }

  const float minus2PIDividedbyResultSize = -2 * PI / resultSize;
  for (size_t i = 0; i < resultSize; ++i) {

    float rightSide = 0;
    float rightSideImaginary = 0;
    {
      // cos(- 2π/n · k) + i · sin(-2π/ n · k)
      const float factor = cos(minus2PIDividedbyResultSize * (i % (resultSize / 2)));
      const float factorImaginary = sin(minus2PIDividedbyResultSize * (i % (resultSize / 2)));

      const float odd = resultOdd[i % (resultSize / 2)];
      const float oddImaginary = resultOddImaginary[i % (resultSize / 2)];

      //(a[r]+a[i])(c[r]+c[i]) = a[r]·c[r] - a[i]·c[i] + i·(a[r]·c[i]+a[i]·c[r]);
      rightSide = factor * odd;
      rightSideImaginary =
          -factorImaginary * oddImaginary + factor * oddImaginary + factorImaginary * odd;
    }

    float result = resultEven[i % (resultSize / 2)];
    float resultImaginary = resultEvenImaginary[i % (resultSize / 2)];
    if (i < resultSize / 2) {
      result += rightSide;
      resultImaginary += rightSideImaginary;
    } else {
      result -= rightSide;
      resultImaginary -= rightSideImaginary;
    }

    fprintf(stdout, "%f %f*i\n", result, resultImaginary);
    fprintf(stderr, "Wrote result number %zu! My pid is: %d\n", i, (int)getpid());
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
