# Author Markus Krainz
# Date 2018
# Builds client and server which are a HTTP client and a HTTP server respectively

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_VID_SOURCE -D_POSIX_C_SOURCE=200809L
#CFLAGS = -Wall -g3 -std=c99 -pedantic $(DEFS)
CFLAGS = -Wall -g -Werror -std=c99 -pedantic -fsanitize=address $(DEFS)
LDFLAGS = -lasan
#LDFLAGS =

.PHONY: all clean docs

all: forkFFT

forkFFT: forkFFT.o tools.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

forkFFT.o: forkFFT.c tools.h
tools.o: tools.c tools.h

docs:  html/index.html

html/index.html: forkFFT.c forkFFT.h tools.c tools.h
	doxygen Doxyfile

clean:
	rm -rf *.o forkFFT html latex