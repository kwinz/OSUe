CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_VID_SOURCE -D_POSIX_C_SOURCE=200809L
#CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
CFLAGS = -Wall -g -Werror -std=c99 $(DEFS)

.PHONY: all clean

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

clean:
	rm -rf *.o client server