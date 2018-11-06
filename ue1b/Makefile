CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_VID_SOURCE -D_POSIX_C_SOURCE=200809L
#CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
CFLAGS = -Wall -g -std=c99 $(DEFS)

.PHONY: all clean

all: client server

client: client.o
	$(CC) $(LDFLAGS) -o $@ $^

server: server.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

server.o: server.c
client.o: client.c

clean:
	rm -rf *.o client server