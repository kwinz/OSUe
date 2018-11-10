#include "tools.h"

int8_t startsWith(const char *longstring, const char *begin) {
  if (strncmp(longstring, begin, strlen(begin)) == 0)
    return 1;
  return 0;
}