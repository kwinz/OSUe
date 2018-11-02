#include "ispalindrom.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // strerror(errno)
  // fprintf(stderr, "[%s] ERROR errmsg: \r\n", argv[0]);

  const char *optstring = "sio:";
  int c;

  char *outfile_arg = NULL;
  int s_count = 0, i_count = 0, o_count = 0;

  // getopt returns -1 if there is no more character
  // Or it returns '?' in case of unknown option or missing option argument
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch (c) {
    case 's': {
      ++s_count;
    } break;
    case 'i': {
      ++i_count;
    } break;
    case 'o': {
      ++o_count;
      outfile_arg = optarg;
    } break;
    case '?': {
      assert(0 && "olol");
    } break;
    default:
      assert(0 && "[%s] We should not reach this if the optstring is valid");
    }
  }

  const int number_of_file_args = argc - optind;
  printf("%d\r\n", number_of_file_args);
  fflush(stdout);

  FILE *input_file = fopen("file1.txt", "r");

  if (input_file == NULL) {

    fprintf(stderr, "fopen failed,: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // warning: linebuffer_size is not const, as getline might modify it when it
  // needs to resize the buffer
  int linebuffer_size = 2;
  char *line = malloc(linebuffer_size);

  if (line == NULL) {
    assert(0 && "malloc failed");
  }

  // getline() returns -1 on failure to read a line (including EOF).
  ssize_t characters_read;
  while ((characters_read = getline(&line, &linebuffer_size, input_file)) !=
         -1) {
    puts(line);
  }

  puts("closing");

  fclose(input_file);

  return EXIT_SUCCESS;
}

/**
 * @brief returns != 0 if the string is a palindrom
 *
 * @param c_string pointer to '\0' terminated char sequence which will be tested
 * for being a palisndrom
 * @param
 */
int8_t isPalindrom(char *c_string, int8_t ignore_case,
                   int8_t ignore_whitespace) {

  return 1;
}