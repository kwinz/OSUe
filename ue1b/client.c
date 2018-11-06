
#include <assert.h>
#include <ctype.h>
#include <errno.h>
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

int8_t startsWith(const char *longstring, const char *begin) {
  if (strncmp(longstring, begin, strlen(begin)) == 0)
    return 1;
  return 0;
}

int main(int argc, char *argv[]) {

  // parse arguments
  char *url = NULL;
  char *port_string = "80", *file_string, *dir_string;
  int port_count = 0, file_count = 0, dir_count = 0;
  char filestringFinal[1024] = {};
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

    char *host = strchr(url, '/');
    if (host == NULL) {
      fprintf(stderr, "[%s, %s, %d] invalid URL \n", argv[0], __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
    host += 2;
    char *directory = strchr(host, '/');
    if (directory == NULL) {
      directory = "";
    } else {
      // use the space of where the '/' was to store the '\0' of the host string
      *directory = '\0';
      directory++;
    }

    fprintf(stderr, "[%s, %s, %d]  host is %s \n", argv[0], __FILE__, __LINE__, host);
    fprintf(stderr, "[%s, %s, %d]  directory is %s \n", argv[0], __FILE__, __LINE__, directory);

    if (dir_count == 1) {
      // fixme check length
      strcpy(filestringFinal, dir_string);
      // fixme check length
      strcat(filestringFinal, "/");

      // see https://tuwel.tuwien.ac.at/mod/forum/discuss.php?d=123112
      char *lastSlashInDirectory = strrchr(directory, '/');
      if (lastSlashInDirectory != NULL) {
        // the directory is more than just a filename we have to parse it
        if (strlen(lastSlashInDirectory) == 1) {
          // the directory is just a folder so append index.html
          strcat(filestringFinal, "index.html");
        }
      } else {
        if (strlen(lastSlashInDirectory) == 0) {
          // the directory is just a folder so append index.html
          strcat(filestringFinal, "index.html");
        } else {
          strcat(filestringFinal, directory);
        }
      }

      // FIXME don't hardcode character array
    }

    if (file_count == 1) {
      strcpy(filestringFinal, file_string);
    }

    struct addrinfo hints;
    struct addrinfo *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /////
    int res = getaddrinfo(host, port_string, &hints, &ai);
    if (res != 0) {
      fprintf(stderr, "[%s, %s, %d]  ERROR could not resolve host \n", argv[0], __FILE__,
              __LINE__);
      exit(EXIT_FAILURE);
    }

    // int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

    if (sockfd < 0) {
      fprintf(stderr, "[%s, %s, %d]  ERROR creating socket \n", argv[0], __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    // struct sockaddr_in server_addr;
    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(80);
    // inet_aton("216.58.201.67", &server_addr.sin_addr.s_addr);

    if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
      fprintf(stderr, "[%s, %s, %d]  ERROR connecting \n", argv[0], __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }

    freeaddrinfo(ai);

    FILE *sockfile = fdopen(sockfd, "r+");

    fprintf(sockfile, "GET /%s HTTP/1.1\r\n", directory);
    fprintf(sockfile, "Host: %s\r\n", host);
    fprintf(sockfile, "Connection: close\r\n\r\n");
    fflush(sockfile); // send all buffered data

    char buf[10240];
    // FIXME: check for NULL return
    fgets(buf, sizeof(buf), sockfile);

    char *httpString = "HTTP/1.1";

    if (!startsWith(buf, httpString)) {
      fprintf(stderr, "[%s, %s, %d]  protocol error must start with %s \n", argv[0], __FILE__,
              __LINE__, httpString);
      exit(EXIT_FAILURE);
    }

    char *afterStatusCode;
    char *afterHttpString = buf + strlen(httpString);
    long response_code = strtol(afterHttpString, &afterStatusCode, 0);

    fprintf(stderr, "[%s, %s, %d]  got HTTP return code %ld , being %s \n", argv[0], __FILE__,
            __LINE__, response_code, afterStatusCode);

    if (response_code != 200) {
      // FIXME textual description of response code
      fprintf(stderr, "[%s, %s, %d] HTTP response code is not 200 \n", argv[0], __FILE__,
              __LINE__);
      exit(3);
    }

    FILE *targetFile = stderr;
    FILE *outputFile = stdout;

    if (strlen(filestringFinal) > 0) {
      outputFile = fopen(filestringFinal, "w");
      if (outputFile == NULL) {
        fprintf(stderr, "[%s, %s, %d]  could not open outfile %s \n", argv[0], __FILE__, __LINE__,
                filestringFinal);
        exit(EXIT_FAILURE);
      }
    }

    while (fgets(buf, sizeof(buf), sockfile) != NULL) {
      // empty line still has two characters for new line
      if (targetFile == stderr && strlen(buf) == 2) {
        targetFile = outputFile;
        fprintf(stderr, "[%s, %s, %d] header ended \n", argv[0], __FILE__, __LINE__);
      }
      fputs(buf, targetFile);
    }

    if (outputFile != stdout) {
      fclose(outputFile);
    }

    exit(EXIT_SUCCESS);
  }
}