# Author Markus Krainz
# Date 2018
# Builds client and server which are a HTTP client and a HTTP server respectively

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_VID_SOURCE -D_POSIX_C_SOURCE=200809L
#CFLAGS = -Wall -g3 -std=c99 -pedantic $(DEFS)
#CFLAGS = -Wall -g -Werror -std=c99 -fsanitize=address -pedantic $(DEFS)
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lasan -lm -lrt -lpthread
#LDFLAGS =

.PHONY: all clean docs

all: supervisor generator

supervisor: supervisor.o tools.o
	$(CC) $(LDFLAGS) -o $@ $^

generator: generator.o tools.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

supervisor.o: supervisor.c tools.h
generator.o: generator.c tools.h

docs:  html/index.html

html/index.html: supervisor.c generator.c tools.h
	doxygen Doxyfile

clean:
	rm -rf *.o supervisor generator html latex