#include "ispalindrom.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int8_t isPalindrom(char *c_string, int8_t ignore_case, int8_t ignore_whitespace);
static void handleFile(FILE *input_file, FILE *out_file, int8_t ignore_case,
                       int8_t ignore_whitespace);
static void printUsage(char *name);

int main(int argc, char *argv[]) {

  // parse arguments
  char *outfile_arg = NULL;
  int ignorewhitespace_count = 0, ignorecase_count = 0, o_count = 0;
  {
    const char *optstring = "sio:";
    int c;

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
        fprintf(stderr, "[%s, %s, %d] ERROR unknown option or missing argument \n", argv[0],
                __FILE__, __LINE__);
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
      } break;
      default:
        assert(0 && "We should never reach this if the optstring is valid");
      }
    }

    if (ignorewhitespace_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-s' argument \n", argv[0],
              __FILE__, __LINE__);
      printUsage(argv[0]);
      exit(EXIT_FAILURE);
    }

    if (ignorecase_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-i' argument \n", argv[0],
              __FILE__, __LINE__);
      printUsage(argv[0]);
      exit(EXIT_FAILURE);
    }

    if (o_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-o' argument \n", argv[0],
              __FILE__, __LINE__);
      printUsage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  FILE *out_file = NULL;

  if (o_count > 0) {
    out_file = fopen(outfile_arg, "w");
    if (out_file == NULL) {
      fprintf(stderr, "[%s, %s, %d]  ERROR fopen failed on outfile: %s\n", argv[0], __FILE__,
              __LINE__, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  const int number_of_file_args = argc - optind;

  if (number_of_file_args == 0) {
    handleFile(NULL, out_file, ignorecase_count, ignorewhitespace_count);
  } else {
    for (int i = 0; i < number_of_file_args; ++i) {
      FILE *input_file = fopen(argv[optind + i], "r");

      if (input_file == NULL) {
        if (out_file != NULL) {
          fclose(out_file);
        }
        fprintf(stderr, "[%s,%s, %d] ERROR fopen failed,: %s\n", argv[0], __FILE__, __LINE__,
                strerror(errno));
        exit(EXIT_FAILURE);
      }

      handleFile(input_file, out_file, ignorecase_count, ignorewhitespace_count);
      fclose(input_file);
    }
  }

  if (out_file != NULL) {
    fclose(out_file);
  }

  return EXIT_SUCCESS;
}

/**
 * @brief Prints help including arguments of this program to stderr.
 *
 * @name: c_string of the name of the executable
 */
void printUsage(char *name) {
  fprintf(stderr, "\nUsage:\n\n");
  fprintf(stderr, "%s [-s] [-i] [-o outfile] [file...]\n", name);
  fprintf(stderr, "\t-o output is written to the specified file\n");
  fprintf(stderr, "\t-s causes program to ignore whitespaces\n");
  fprintf(stderr, "\t-i program does not differentiate between lower and upper cases letters.\n");
}

/**
 * @brief Checks one input file's lines for being a palindrome.
 *
 * @detail The file is read line by line. Memory is allocated dynamicaly internally.
 * Output is written to *out_file or stdout.
 *
 * @param input_file filestream to be read. Must be opened and valid. This parameter may be NULL -
 * in this case this function reads from stdin
 * @param out_file filestream to write result to. Must be opened and valid. If this is NULL then
 * this funtion outputs to stdout
 * @param ignore_case if != 0 then then upper/lower-case is ignored when processing
 * palindrome
 * @param ignore_whitespace if != 0 then then all whitespace (not including special
 * characters like '\t') is ignored when processing palindrome
 */
void handleFile(FILE *input_file, FILE *out_file, int8_t ignore_case, int8_t ignore_whitespace) {

  // note: linebuffer_size is not const, as getline might modify it when it
  // needs to resize the buffer
  size_t linebuffer_size = 2;
  char *line = malloc(linebuffer_size);

  if (line == NULL) {
    if (input_file != NULL) {
      fclose(input_file);
    }
    if (out_file != NULL) {
      fclose(out_file);
    }

    fprintf(stderr, "FATAL ERROR out of memory");
    exit(EXIT_FAILURE);
  }

  // getline() returns -1 on failure to read a line (including EOF).
  ssize_t characters_read;
  while ((characters_read =
              getline(&line, &linebuffer_size, input_file == NULL ? stdin : input_file)) != -1) {
    // remove tailing newline (which is also the first occurance of '\n' because
    // we read line by line)
    {
      char *pos = strchr(line, '\n');
      *pos = '\0';
    }

    // check if string is palindrome and print the result
    {
      const uint8_t is_palindrom = isPalindrom(line, ignore_case, ignore_whitespace);
      fprintf(out_file == NULL ? stdout : out_file, "%s %s \n", line,
              is_palindrom ? "is a palindrom" : "is not a palindrom");
    }
  }

  free(line);
}

/**
 * @brief returns != 0 if the string is a palindrom
 *
 * @detail May skip spaces and match different cases as equal depending on parameters.
 *
 * @param c_string pointer to '\0' terminated char sequence which will be tested
 * for being a palindrom
 * @param ignore_case if != 0 then then upper/lower-case is ignored when processing
 * palindrome
 * @param ignore_whitespace if != 0 then then all whitespace (not including special
 * characters like '\t') is ignored when processing palindrome
 * @return 1 if c_string is a palindrome, 0 otherwise
 */
int8_t isPalindrom(char *c_string, int8_t ignore_case, int8_t ignore_whitespace) {

  int i = 0;
  int j = strlen(c_string) - 1;

  while (i < j) {
    if (ignore_whitespace) {
      // ignore whitespace from left
      while (c_string[i] == ' ') {
        ++i;
        if (i > j) {
          return 1;
        }
      }

      // ignore whitespace from right
      while (c_string[j] == ' ') {
        --j;
        if (i > j) {
          return 1;
        }
      }
    }

    if (ignore_case) {
      if (tolower(c_string[i]) != tolower(c_string[j])) {
        return 0;
      }
    } else {
      if (c_string[i] != c_string[j]) {
        return 0;
      }
    }

    ++i;
    --j;
  }
  return 1;
}