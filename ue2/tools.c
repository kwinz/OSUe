#include <string.h>

/** @defgroup Tools */

/** @addtogroup Tools
 * @brief Provides HTTP Utility Tools
 *
 * @details Right now mostly string manipulation functions.
 *
 * @author Markus Krainz
 * @date November 2018
 *  @{
 */

#include "tools.h"

/**
 * @brief Checks if a string starts with a prefix string.
 *
 * @param longstring c_string that should be checked if it starts with begin
 * @param begin c_string that we want to check if it lies wholy within the first characters of
 * longstring
 * @return 1 if longstring starts with begin, 0 otherwise
 */
int8_t startsWith(const char *longstring, const char *begin) {
  if (strncmp(longstring, begin, strlen(begin)) == 0)
    return 1;
  return 0;
}

/**
 * @brief Checks if a string ends in a suffix string.
 *
 * @detail Based on https://stackoverflow.com/a/41921100/643011
 *
 * @param str c_string that should be checked if it ends with suffix
 * @param suffix c_string that we want to check if it lies wholy within the last characters of
 * longstring
 * @return 1 if longstring ends with suffix, 0 otherwise
 */
int8_t strEndsWith(char *str, char *suffix) {
  int len = strlen(str);
  int suffixlen = strlen(suffix);

  // if suffix is longer than string return 0
  if (suffixlen > len) {
    return 0;
  }

  // move to the end of the string
  str += (len - suffixlen);

  return strcmp(str, suffix) == 0;
}

/**
 * @brief fprintfs to stderr if verbose !=0
 *
 * @param verbose this function does nothing if verbose==0
 * @param fmt printf-style format string
 * @param ... variable arguments for fprintf
 */
void printVerbose(int verbose, const char *fmt, va_list args) {
  if (verbose) {
    fprintf(stderr, fmt, args);
  }
}

/** @}*/