#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "server.h"
#include "tools.h"

volatile sig_atomic_t quit = 0;
volatile sig_atomic_t client_dead = 0;

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

    if (positional_args_count != 1) {
      fprintf(stderr, "[%s, %s, %d]  ERROR DOCROOT is mandatory. \n", argv[0], __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    doc_root = argv[optind];

    fprintf(stderr, "[%s, %s, %d]  DOCROOT is %s \n", argv[0], __FILE__, __LINE__, doc_root);
  }

  long port;
  {
    char *endpointer;
    port = strtol(port_string, &endpointer, 0);
    char *endpointer2 = port_string + strlen(port_string);

    fprintf(stderr, "[%s, %s, %d]  Pointer1 %p \n", argv[0], __FILE__, __LINE__, endpointer);
    fprintf(stderr, "[%s, %s, %d]  Pointer2 %p \n", argv[0], __FILE__, __LINE__, endpointer2);

    // FIXME: set narrower limits
    if (port == LONG_MIN || port == LONG_MAX || (endpointer != endpointer2)) {

      fprintf(stderr, "[%s, %s, %d]  ERROR Could not parse port. \n", argv[0], __FILE__, __LINE__);

      // note: strtol only sets errno, if the long range was exceeded, but not in case of other
      // conversion errors.
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "[%s, %s, %d]  Port is %ld\n", argv[0], __FILE__, __LINE__, port);
  }

  {

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // initialize sa to 0
    sa.sa_handler = &handle_signal;
    sigaction(SIGINT, &sa, NULL);
  }
  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // initialize sa to 0
    sa.sa_handler = &handle_signal;
    sigaction(SIGTERM, &sa, NULL);
  }

  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // initialize sa to 0
    sa.sa_handler = &handle_signal_sigpipe;
    sigaction(SIGPIPE, &sa, NULL);

    // signal(SIGPIPE, SIG_IGN);
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "[%s, %s, %d]  ERROR Could not create socket. %s \n", argv[0], __FILE__,
            __LINE__, strerror(errno));
    exit(EXIT_FAILURE);
  }

  // set SO_REUSEADDR
  {
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
  }

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  memset(&(sa.sin_addr), 0, sizeof sa.sin_addr);
  // inet_aton("63.161.169.137", sa.sin_addr.s_addr);

  if (bind(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
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

  while (!quit) {
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0) {
      if (errno == EINTR) {
        continue;
      }

      fprintf(stderr, "[%s, %s, %d]  ERROR Error while accepting incomming request. %s \n",
              argv[0], __FILE__, __LINE__, strerror(errno));
      exit(EXIT_FAILURE);
    }

    client_dead = 0;

    // note: use connfd, not sockfd!!
    FILE *sockfile = fdopen(connfd, "r+");
    if (sockfile == NULL) {
      fprintf(stderr, "[%s, %s, %d]  ERROR Could not open stream with incoming connection. %s \n",
              argv[0], __FILE__, __LINE__, strerror(errno));
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "[%s, %s, %d]  Got client. Reading.. \n", argv[0], __FILE__, __LINE__);

    char bufFirstline[1024];
    char *request_method_string, *path_file_string, *protocol_string;

    if (fgets(bufFirstline, sizeof(bufFirstline), sockfile) != NULL) {
      request_method_string = strtok(bufFirstline, " ");
      if (request_method_string == NULL) {
        fprintf(stderr, "[%s, %s, %d] ERROR Problem with request_method_string. \n", argv[0],
                __FILE__, __LINE__);
        send400(connfd, sockfile);
      } else {
        fprintf(stderr, "[%s, %s, %d] request_method_string %s \n", argv[0], __FILE__, __LINE__,
                request_method_string);
        path_file_string = strtok(NULL, " ");
        if (path_file_string == NULL) {
          fprintf(stderr, "[%s, %s, %d] ERROR Problem with path_file_string \n", argv[0], __FILE__,
                  __LINE__);
          send400(connfd, sockfile);
        } else {
          fprintf(stderr, "[%s, %s, %d] path_file_string %s \n", argv[0], __FILE__, __LINE__,
                  path_file_string);
          protocol_string = strtok(NULL, " ");
          if (path_file_string == NULL) {
            fprintf(stderr, "[%s, %s, %d] ERROR Problem with protocol_string \n", argv[0],
                    __FILE__, __LINE__);
            send400(connfd, sockfile);
          } else {
            fprintf(stderr, "[%s, %s, %d] protocol_string %s \n", argv[0], __FILE__, __LINE__,
                    protocol_string);

            if (!startsWith(protocol_string, "HTTP/1.1")) {
              fprintf(stderr, "[%s, %s, %d] protocol_string %zu %zu \n", argv[0], __FILE__,
                      __LINE__, strlen(protocol_string), strlen("HTTP/1.1"));
              fprintf(stderr, "[%s, %s, %d] ERROR invalid protocol_string \n", argv[0], __FILE__,
                      __LINE__);
              send400(connfd, sockfile);
            } else {
              if (strcmp(request_method_string, "GET") != 0) {
                fprintf(stderr, "[%s, %s, %d] request_method_string %s \n", argv[0], __FILE__,
                        __LINE__, request_method_string);
                fprintf(stderr, "[%s, %s, %d] ERROR Can't handle this request. \n", argv[0],
                        __FILE__, __LINE__);
                send501(connfd, sockfile);
              }
            }
          }
        }
      }
    }

    // read all headers
    char buf[1024];
    while (fgets(buf, sizeof(buf), sockfile) != NULL) {
      fprintf(stderr, "[%s, %s, %d] Read %s \n", argv[0], __FILE__, __LINE__, buf);
      if (strlen(buf) == 2) {
        break;
      }
    }

    drainBuffer(connfd, sockfile);

    FILE *inFile;
    char filestringFinal[1000];
    {

      // FIXME bufferoverflow protection

      fprintf(stderr, "[%s, %s, %d] path_file_string is %s \n", argv[0], __FILE__, __LINE__,
              path_file_string);

      strcpy(filestringFinal, doc_root);
      strcat(filestringFinal, path_file_string);

      // FIXME put into utility function
      if (path_file_string && *path_file_string != '\0' &&
          '/' == *(path_file_string + strlen(path_file_string) - 1)) {
        fprintf(stderr, "[%s, %d] path_file_string was a directory \n", __FILE__, __LINE__);
        strcat(filestringFinal, indexfile_string);
      }

      inFile = fopen(filestringFinal, "rb");
      if (inFile == NULL) {
        fprintf(stderr, "[%s, %s, %d]  could not open outfile %s \n", argv[0], __FILE__, __LINE__,
                filestringFinal);

        send404(connfd, sockfile);
        // FIXME handle next request instead
        return EXIT_SUCCESS;
      }
    }

    fprintf(stderr, "[%s, %s, %d] Opened file. Sending %s \n", argv[0], __FILE__, __LINE__,
            filestringFinal);

    // send file
    {
      fprintf(sockfile, "HTTP/1.1 200 OK\r\n");

      // e.g. Date: Sun, 11 Nov 18 22:55:00 GMT
      {
        char date[1000];
        time_t now = time(0);
        struct tm tm = *gmtime(&now);

        // I hate implementing the years as 2 digits, because RFC822 is obsolete
        // https://tools.ietf.org/html/rfc7231#section-7.1.1.1
        // https://www.ietf.org/rfc/rfc3339.txt
        // but the exercise specification is forcing me to.

        strftime(date, sizeof date, "%a, %d %b %y %H:%M:%S %Z", &tm);
        fprintf(sockfile, "Date: %s\r\n", date);
      }
      {
        fseek(inFile, 0, SEEK_END); // seek to end of file
        long size = ftell(inFile);  // get current file pointer
        fseek(inFile, 0, SEEK_SET);
        fprintf(sockfile, "Content-Length: %ld\r\n", size);
      }
      fprintf(sockfile, "Connection: close\r\n\r\n");

      {
        char copy_buffer[1024];
        size_t bytes;
        while (!client_dead && 0 < (bytes = fread(copy_buffer, 1, sizeof(copy_buffer), inFile))) {
          fwrite(copy_buffer, 1, bytes, sockfile);
        }
      }
    }

    // closing stream
    { fclose(sockfile); }

    fprintf(stderr, "[%s, %s, %d]  Finished serving client request. Errno: %s\n", argv[0],
            __FILE__, __LINE__, strerror(errno));
  }

  fprintf(stderr, "[%s, %s, %d]  Finished executing. Errno: %s \n", argv[0], __FILE__, __LINE__,
          strerror(errno));
  return EXIT_SUCCESS;
}

void send400(int fd, FILE *sockfile) {
  sendResponseHeaderOnly(fd, sockfile, "HTTP/1.1 400 Bad Request\r\n");
}

void send404(int fd, FILE *sockfile) {
  sendResponseHeaderOnly(fd, sockfile, "HTTP/1.1 404 Not Found\r\n");
}

void send501(int fd, FILE *sockfile) {
  sendResponseHeaderOnly(fd, sockfile, "HTTP/1.1 501 Not implemented\r\n");
}

void handle_signal(int signal) { quit = 1; }

void handle_signal_sigpipe(int signal) { client_dead = 1; }

void sendResponseHeaderOnly(int fd, FILE *sockfile, char *response_string) {
  drainBuffer(fd, sockfile);
  fprintf(sockfile, response_string);
  fprintf(sockfile, "Connection: close\r\n\r\n");
  // send all buffered data (but sockfile stream is probably unbuffered anyway )
  fflush(sockfile);
}

void drainBuffer(int fd, FILE *sockfile) {
  fprintf(stderr, "[%s, %d] Before draining buffer \n", __FILE__, __LINE__);

  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  static char buf[1024];
  size_t bytes_read;
  while ((bytes_read = fread(buf, sizeof(char), sizeof(buf) / sizeof(char), sockfile)) != 0) {
    fprintf(stderr, "[%s, %d] Read %zu bytes \n", __FILE__, __LINE__, bytes_read);
  }
  fprintf(stderr, "[%s, %d] Read %zu bytes \n", __FILE__, __LINE__, bytes_read);

  fprintf(stderr, "[%s, %d] Setting blocking again \n", __FILE__, __LINE__);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

  fprintf(stderr, "[%s, %d] After draining buffer \n", __FILE__, __LINE__);
}