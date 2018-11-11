#include <string.h>

#include "tools.h"

int8_t startsWith(const char *longstring, const char *begin) {
  if (strncmp(longstring, begin, strlen(begin)) == 0)
    return 1;
  return 0;
}

// based on https://stackoverflow.com/a/41921100/643011
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