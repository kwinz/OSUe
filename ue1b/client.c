
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

#include <unistd.h>
#include <zlib.h>

#include "client.h"
#include "tools.h"

int main(int argc, char *argv[]) {

  // parse arguments
  char *url = NULL;
  char *port_string = "80", *file_string, *dir_string;
  int port_count = 0, file_count = 0, dir_count = 0;
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

    char filestringFinal[1024];
    {
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

    fprintf(sockfile, "Accept-Encoding: gzip\r\n");

    fprintf(sockfile, "Connection: close\r\n\r\n");
    fflush(sockfile); // send all buffered data

    char buf[1024];
    // FIXME: check for NULL return
    fgets(buf, sizeof(buf), sockfile);

    char *httpString = "HTTP/1.1";

    if (!startsWith(buf, httpString)) {
      fprintf(stderr, "[%s, %s, %d] ERROR Protocol error! Must start with %s \n", argv[0],
              __FILE__, __LINE__, httpString);
      exit(EXIT_FAILURE);
    }

    char *afterStatusCode;
    char *afterHttpString = buf + strlen(httpString);
    long response_code = strtol(afterHttpString, &afterStatusCode, 0);

    fprintf(stderr, "[%s, %s, %d] Got HTTP return code %ld , being %s \n", argv[0], __FILE__,
            __LINE__, response_code, afterStatusCode);

    if (response_code != 200) {
      // FIXME textual description of response code
      fprintf(stderr, "[%s, %s, %d] HTTP response code is not 200 \n", argv[0], __FILE__,
              __LINE__);
      exit(3);
    }

    FILE *outputFile = stdout;

    if (strlen(filestringFinal) > 0) {
      outputFile = fopen(filestringFinal, "w");
      if (outputFile == NULL) {
        fprintf(stderr, "[%s, %s, %d]  could not open outfile %s \n", argv[0], __FILE__, __LINE__,
                filestringFinal);
        exit(EXIT_FAILURE);
      }
    }

    int8_t server_used_gzip = 0;
    while (fgets(buf, sizeof(buf), sockfile) != NULL) {
      fputs(buf, stderr);

      if (startsWith(buf, "Content-Encoding: gzip")) {
        fprintf(stderr, "[%s, %s, %d] Server sending gziped content \n", argv[0], __FILE__,
                __LINE__);
        server_used_gzip = 1;
      }

      // empty line still has two characters for new line
      if (strlen(buf) == 2) {
        fprintf(stderr, "[%s, %s, %d] header ended \n", argv[0], __FILE__, __LINE__);
        break;
      }
    }

    if (!server_used_gzip) {
      char copy_buffer[1024];
      size_t bytes;
      while (0 < (bytes = fread(copy_buffer, 1, sizeof(copy_buffer), sockfile))) {
        fwrite(copy_buffer, 1, bytes, outputFile);
      }
    } else {

      uint8_t copy_buffer[10240];
      uint8_t copy_buffer_decompressed[300];
      size_t bytes;

      int ret;
      z_stream zs;
      memset(&zs, 0, sizeof(zs));
      inflateInit2(&zs, MAX_WBITS + 16);

      while (0 < (bytes = fread(copy_buffer, 1, sizeof(copy_buffer), sockfile))) {

        fprintf(stderr, "[%s, %s, %d]  Read once, size is %zu \n", argv[0], __FILE__, __LINE__,
                bytes);

        zs.next_in = copy_buffer;
        zs.avail_in = bytes;

        do {
          zs.next_out = copy_buffer_decompressed;
          zs.avail_out = sizeof(copy_buffer_decompressed);

          ret = inflate(&zs, Z_SYNC_FLUSH);
          assert(ret != Z_STREAM_ERROR); /* state not clobbered */

          const size_t have = sizeof(copy_buffer_decompressed) - zs.avail_out;
          fprintf(stderr, "[%s, %s, %d]  Inflated once, size is %zu \n", argv[0], __FILE__,
                  __LINE__, have);

          fwrite(copy_buffer_decompressed, 1, have, outputFile);
        } while (zs.avail_out == 0);
      }
      deflateEnd(&zs);
    }

    if (outputFile != stdout) {
      fclose(outputFile);
    }

    exit(EXIT_SUCCESS);
  }
}