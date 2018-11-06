
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  // parse arguments
  char *url = NULL;
  char *port_string, *file_string, *dir_string;
  int port_count = 0, file_count = 0, dir_count = 0;
  {
    const char *optstring = "p:o:d:";
    int c;

    // getopt returns -1 if there is no more character
    // Or it returns '?' in case of unknown option or missing option argument
    while ((c = getopt(argc, argv, optstring)) != -1) {
      switch (c) {
      case 'p': {
        ++port_count;
        port_string = optarg;
      } break;
      case 'o': {
        ++file_count;
        file_string = optarg;
      } break;
      case 'd': {
        ++dir_count;
        dir_string = optarg;
      } break;
      case '?': {
        fprintf(stderr, "[%s, %s, %d] ERROR unknown option or missing argument \n", argv[0],
                __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      } break;
      default:
        assert(0 && "We should never reach this if the optstring is valid");
      }
    }

    if (port_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-p' argument \n", argv[0],
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    if (file_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-o' argument \n", argv[0],
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    if (dir_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-d' argument \n", argv[0],
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    if (file_count > 0 && dir_count > 0) {
      fprintf(stderr,
              "[%s, %s, %d]  ERROR Provide either -o FILE or -d DIR argument but not both \n",
              argv[0], __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    const int positional_args_count = argc - optind;

    if (positional_args_count != 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide mandatory URL parameter \n", argv[0], __FILE__,
              __LINE__);
      exit(EXIT_FAILURE);
    }

    url = argv[optind];

    fprintf(stderr, "[%s, %s, %d]  url is %s \n", argv[0], __FILE__, __LINE__, url);

    exit(EXIT_SUCCESS);
  }
}