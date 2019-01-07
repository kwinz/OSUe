#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  fprintf(stderr, "edges: %d\n", argc - 1);

  if (argc == 1) {
    fprintf(stderr, "Provide at least one edge. \n");
    return EXIT_FAILURE;
  }

  // parse edges from arguments
  int max_vert = 0;
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

    max_vert = fmax(a, max_vert);
    max_vert = fmax(b, max_vert);
  }

  fprintf(stderr, "vertices: %d\n", max_vert + 1);

  // create array of vertices
  int vertices[max_vert + 1];
  for (int i = 0; i <= max_vert; ++i) {
    vertices[i] = i;
  }

  for (int i = 0; i <= max_vert; ++i) {
    fprintf(stderr, "%d,", vertices[i]);
  }

  fprintf(stderr, "\nBEFORE SHUFFLE\n");

  // shuffle
  srand(time(NULL));
  for (int i = 0; i < max_vert - 1; ++i) {
    const int j = i + rand() % (max_vert + 1 - i);
    fprintf(stderr, "swapping indizes: %d,%d\n", i, j);
    const int temp = vertices[i];
    vertices[i] = vertices[j];
    vertices[j] = temp;

    for (int i = 0; i <= max_vert; ++i) {
      fprintf(stderr, "%d,", vertices[i]);
    }
    fprintf(stderr, "\n----\n");
  }

  fprintf(stderr, "AFTER SHUFFLE\n");

  return EXIT_SUCCESS;
}