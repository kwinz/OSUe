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
  int ignorewhitespace_count = 0, ignorecase_count = 0, o_count = 0;

  // getopt returns -1 if there is no more character
  // Or it returns '?' in case of unknown option or missing option argument
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch (c) {
    case 's': {
      ++ignorewhitespace_count;
    } break;
    case 'i': {
      ++ignorecase_count;
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
  while ((characters_read = getline(&line, &linebuffer_size, input_file)) != -1) {
    // remove tailing newline (which is also the first occurance of '\n' because
    // we read line by line)
    {
      char *pos = strchr(line, '\n');
      *pos = '\0';
    }

    // check if string is palindrome and print the result
    {
      const uint8_t is_palindrom = isPalindrom(line, ignorecase_count, ignorewhitespace_count);
      printf("%s %s \n", line, is_palindrom ? "is a palindrom" : "is not a palindrom");
    }
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
int8_t isPalindrom(char *c_string, int8_t ignore_case, int8_t ignore_whitespace) {

  int i = 0;
  int j = strlen(c_string) - 1;

  while (i < j) {
    if (c_string[i] != c_string[j]) {
      return 0;
    }
    --i;
    --j;
  }
  return 1;
}