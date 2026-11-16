CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -O2 -pthread
AR = ar
ARFLAGS = rcs

all: client server

libcommon.a: common.o
	$(AR) $(ARFLAGS) $@ $^

client: client.o libcommon.a
	$(CC) $(CFLAGS) -o $@ client.o libcommon.a

server: server.o libcommon.a
	$(CC) $(CFLAGS) -o $@ server.o libcommon.a

client.o: client.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

server.o: server.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

common.o: common.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o libcommon.a client server

.PHONY: all clean