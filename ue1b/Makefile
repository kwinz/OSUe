# Author Markus Krainz
# Date 2018
# Builds client and server which are a HTTP client and a HTTP server respectively

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_VID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
#CFLAGS = -Wall -g -Werror -std=c99 -pedantic -fsanitize=address $(DEFS)
LDFLAGS = -lasan -lz

.PHONY: all clean docs

all: client server

client: client.o tools.o
	$(CC) $(LDFLAGS) -o $@ $^

server: server.o tools.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

server.o: server.c server.h tools.h
client.o: client.c client.h tools.h
tools.o: tools.c tools.h

docs:  html/index.html

html/index.html: server.c server.h client.c client.h tools.c tools.h
	doxygen Doxyfile

clean:
	rm -rf *.o client server html latex