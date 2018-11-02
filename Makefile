CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_VID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

OBJECTS main.o

.PHONY: all clean

all: ispalindrom

ispalindrom: $(OBJECTS)
    $(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
    $(CC) $(CFLAGS) -c -o $@ $<

main.o main.c main.h 

clean:
    rm -rf *.o ue1