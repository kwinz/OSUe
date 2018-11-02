#include "ispalindrom.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // strerror(errno)
  fprintf(stderr, "[%s] ERROR errmsg: \r\n", argv[0]);

  const char *optstring = "";
  int c;
  while ((c = getopt(argc, argv, optstring)) != -1) {
    switch (c) {
    case 's': {
    } break;
    case 'i': {

    } break;
    case '?': {

    } break;
    default:
      assert(0 && "[%s] We should not reach this if the optstring is valid");
    }
  }

  return EXIT_SUCCESS;
}