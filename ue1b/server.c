#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {

  // parse arguments
  char *doc_root = NULL;
  char *port_string = "8080", *indexfile_string = "index.html";
  int port_count = 0, indexfile_count = 0;

  {
    const char *optstring = "p:i:";
    int c;

    // getopt returns -1 if there is no more character
    // Or it returns '?' in case of unknown option or missing option argument
    while ((c = getopt(argc, argv, optstring)) != -1) {
      switch (c) {
      case 'p': {
        ++port_count;
        port_string = optarg;
      } break;
      case 'i': {
        ++indexfile_count;
        indexfile_string = optarg;
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

    if (indexfile_count > 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Provide at most one '-o' argument \n", argv[0],
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    const int positional_args_count = argc - optind;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "[%s, %s, %d]  ERROR Could not create socket. %s \n", argv[0], __FILE__,
            __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  char *endpointer;
  long port = strtol(port_string, &endpointer, 0);
  char *endpointer2 = port_string + strlen(port_string);

  fprintf(stderr, "[%s, %s, %d]  Pointer1 %p \n", argv[0], __FILE__, __LINE__, endpointer);
  fprintf(stderr, "[%s, %s, %d]  Pointer2 %p \n", argv[0], __FILE__, __LINE__, endpointer2);

  //FIXME: set narrower limits
  if (port == LONG_MIN || port == LONG_MAX || (endpointer != endpointer2)) {

    fprintf(stderr, "[%s, %s, %d]  ERROR Could not parse port. \n", argv[0], __FILE__, __LINE__);

    // note: strtol only sets errno, if the long range was exceeded, but not in case of other
    // conversion errors.
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "[%s, %s, %d]  Port is %ld\n", argv[0], __FILE__, __LINE__, port);

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  memset(&(sa.sin_addr), 0, sizeof sa.sin_addr);
  // inet_aton("63.161.169.137", sa.sin_addr.s_addr);

  if (bind(sockfd, &sa, sizeof(struct sockaddr_in)) < 0) {
    fprintf(stderr, "[%s, %s, %d]  ERROR Could not bind socket. %s \n", argv[0], __FILE__,
            __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (listen(sockfd, 1) < 0) {
    fprintf(
        stderr,
        "[%s, %s, %d]  ERROR Could not mark socket as passive, listening for connections. %s \n",
        argv[0], __FILE__, __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "[%s, %s, %d]  Waiting for incoming clients. \n", argv[0], __FILE__, __LINE__);

  int connfd = accept(sockfd, NULL, NULL);
  if (connfd < 0) {
    fprintf(stderr, "[%s, %s, %d]  ERROR Error while accepting incomming request. %s \n", argv[0],
            __FILE__, __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  FILE *sockfile = fdopen(sockfd, "r+");

  if (sockfile == NULL) {
    fprintf(stderr, "[%s, %s, %d]  ERROR Could not open stream with incoming connection. %s \n",
            argv[0], __FILE__, __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "[%s, %s, %d]  Got client. Reading.. \n", argv[0], __FILE__, __LINE__);

  char buf[1024];

  while (fgets(buf, sizeof(buf), sockfile) != NULL) {

    fprintf(stderr, "[%s, %s, %d] Read %s \n", argv[0], __FILE__, __LINE__, buf);

    char *token = strtok(buf, " ");
    while (token != NULL) {
      fprintf(stderr, "token: %s\n", token);
      token = strtok(NULL, " ");
    }
  }
  return EXIT_SUCCESS;
}