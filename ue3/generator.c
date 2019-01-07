#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Choose a limit on the maximum number
// of edges which we can process.
#define MAX_EDGES 30

/*
#if (MAX_EDGES) < 255
#define node_t uint8_t
#else // MAX_EDGES > 255
#define node_t uint
#endif // MAX_EDGES > 255
*/

typedef struct Edge {
  int a, b;
} Edge_t;

int main(int argc, char *argv[]) {
  // C99 VLA
  Edge_t graph[argc - 1];
  fprintf(stderr, "arguments %d\n", argc);

  for (int i = 0; i < argc - 1; i++) {
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
  }

  return EXIT_SUCCESS;
}