#include "tools.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <math.h>

/** @defgroup forkFFT */

/** @addtogroup forkFFT
 * @brief Calculates Fast Fourier transform (FFT)
 *
 * @details Recursively creates processes to calculate fast fourier transformation.
 *
 * @author Markus Krainz
 * @date December 2018
 * @{
 */

typedef struct childData {
  int stdin;
  int stdout;
  pid_t pid;
} childData_t;

/**
 * @brief Starts a childprocess to process FFT
 *
 * @detail Starts a new child process executing the same main. Sets up
 * stdin and stdout and returns them together with the child pid in a
 * struct childData.
 * @param argv argv of the current parent process
 * @return A struct with stdin, stdout and pid of the new childprocess
 */
static childData_t setupChild(char *argv[]) {
  int pipePairStdin[2];
  int pipeRet = pipe(pipePairStdin);
  if(pipeRet != 0){
    // an error has occured during creating the pipe for stdin
    exit(EXIT_FAILURE);
  }

  int pipePairStdout[2];
  pipeRet = pipe(pipePairStdout);
  if(pipeRet != 0){
    // an error has occured during creating the pipe for stdout
    exit(EXIT_FAILURE);
  }

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
    // an error has occured during forking
    exit(EXIT_FAILURE);
  }
  // we are a parent

  childData_t ret;

  ret.pid = pid;
  ret.stdin = pipePairStdin[1];
  ret.stdout = pipePairStdout[0];

  close(pipePairStdin[0]);
  close(pipePairStdout[1]);

  return ret;
}

int main(int argc, char *argv[]) {

  fprintf(stderr, "Running main()... My pid is: %d\n", (int)getpid());

  Myvect_t myVect;
  init_myvect(&myVect);

  // read input
  {
    size_t linebufferSize = 0;
    char *line = NULL;
    while ((getline(&line, &linebufferSize, stdin)) != -1) {
      char *endPointer;
      const float valueOfThisLine = strtof(line, &endPointer);
      if(endPointer == NULL || (*endPointer != '\r' && *endPointer != '\n' && *endPointer != '\0')){
        fprintf(stderr, "%s Could not parse input value: %s. My pid is: %d\n", argv[0], line, (int)getpid());
        free(line);
        freedata_myvect(&myVect);
        return EXIT_FAILURE;
      } else{
         fprintf(stderr, "Parsed");
      }
      push_myvect(&myVect, valueOfThisLine);
    }
    free(line);
  }

  fprintf(stderr, "Read %zu floats from stdin. My pid is: %d\n", myVect.size, (int)getpid());

  if (myVect.size == 0) {
    freedata_myvect(&myVect);
    return EXIT_FAILURE;
  }

  if (myVect.size == 1) {
    fprintf(stdout, "%f 0.0*i", myVect.data[0]);
    fprintf(stderr, "Wrote result! My pid is: %d\n", (int)getpid());
    freedata_myvect(&myVect);
    return EXIT_SUCCESS;
  }

  if (myVect.size % 2 != 0) {
    freedata_myvect(&myVect);
    return EXIT_FAILURE;
  }

  childData_t even = setupChild(argv);
  childData_t odd = setupChild(argv);

  fprintf(stderr, "Send data to children...\n");
  // send data to children
  {
    FILE *childEvenFp = fdopen(even.stdin, "w");
    FILE *childOddFp = fdopen(odd.stdin, "w");

    for (size_t i = 0; i < myVect.size; i += 2) {
      fprintf(childEvenFp, "%f\n", myVect.data[i]);
    }
    for (size_t i = 1; i < myVect.size; i += 2) {
      fprintf(childOddFp, "%f\n", myVect.data[i]);
    }

    fclose(childEvenFp);
    fclose(childOddFp);
    close(even.stdin);
    close(odd.stdin);
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
    FILE *childEvenFp = fdopen(even.stdout, "r");
    FILE *childOddFp = fdopen(odd.stdout, "r");
    size_t linebufferSize = 0;
    char *line = NULL;

    for (size_t k = 0; k < resultSize / 2; ++k) {
      char *endPointer;

      int res = getline(&line, &linebufferSize, childEvenFp);
      if (res == -1) {
        fprintf(stderr, "%s Cannot read even! My pid is: %d\n", argv[0], (int)getpid());
        free(line);
        exit(EXIT_FAILURE);
      }
      resultEven[k] = strtof(line, &endPointer);
      resultEvenImaginary[k] = strtof(endPointer, &endPointer);

      res = getline(&line, &linebufferSize, childOddFp);
      if (res == -1) {
        fprintf(stderr, "%s Cannot read odd! My pid is: %d\n", argv[0], (int)getpid());
        free(line);
        exit(EXIT_FAILURE);
      }
      resultOdd[k] = strtof(line, &endPointer);
      resultOddImaginary[k] = strtof(endPointer, &endPointer);
    }

    free(line);

    fclose(childEvenFp);
    fclose(childOddFp);
    close(even.stdout);
    close(odd.stdout);
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
      fprintf(stderr, "%s Cannot wait!\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }

    if (pid == odd.pid) {
      oddDead = true;
    } else if (pid == even.pid) {
      evenDead = true;
    }

    if (oddDead && evenDead) {
      break;
    }
  }

  fprintf(stderr, "Calculating and outputting own results...\n");
  const float minus2PIDividedbyResultSize = -2 * PI / resultSize;
  for (int i = 0; i < resultSize; ++i) {
    const int k = i % (resultSize / 2);

    float rightSide;
    float rightSideImaginary;
    {
      // cos(- 2π/n · k) + i · sin(-2π/ n · k)

      const float factor = cos(minus2PIDividedbyResultSize * k);
      const float factorImaginary = sin(minus2PIDividedbyResultSize * k);

      const float odd = resultOdd[i % (resultSize / 2)];
      const float oddImaginary = resultOddImaginary[i % (resultSize / 2)];

      //(a[r]+a[i])(c[r]+c[i]) = a[r]·c[r] - a[i]·c[i] + i·(a[r]·c[i]+a[i]·c[r]);
      rightSide = factor * odd - factorImaginary * oddImaginary;
      rightSideImaginary = factor * oddImaginary + factorImaginary * odd;
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
    // fprintf(stderr, "Wrote result number %zu! My pid is: %d\n", i, (int)getpid());
  }

  return EXIT_SUCCESS;
}

/** @}*/