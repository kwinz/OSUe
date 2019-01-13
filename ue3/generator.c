#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tools.h"

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

  Edge_t report[MAX_REPORTED];
  srand(time(NULL));

  // shuffle
  for (int i = 0; i < max_vert - 1; ++i) {
    const int j = i + rand() % (max_vert + 1 - i);
    const int temp = vertices[i];
    vertices[i] = vertices[j];
    vertices[j] = temp;
  }

  fprintf(stderr, "shuffled: ");
  for (int i = 0; i <= max_vert; ++i) {
    fprintf(stderr, "%d,", vertices[i]);
  }
  fprintf(stderr, "\n");

  bool max_exceeded = false;

  for (int i = 0, reported = 0; i < edge_count && !max_exceeded; ++i) {
    for (int j = 0; j <= max_vert; ++j) {
      if (graph[i].a == vertices[j]) {
        fprintf(stderr, "-[%d,%d]\n", graph[i].a, graph[i].b);
        break;
      }
      if (graph[i].b == vertices[j]) {
        if (reported == MAX_REPORTED) {
          max_exceeded = true;
          break;
        }
        fprintf(stderr, "+[%d,%d]\n", graph[i].a, graph[i].b);
        report[reported] = graph[i];
        ++reported;
        break;
      }
      assert(j < max_vert && "No matches found yet. This should never happen.");
    }
  }

  fprintf(stderr, "exceeded:%s\n", max_exceeded ? "true" : "false");

  return EXIT_SUCCESS;
}