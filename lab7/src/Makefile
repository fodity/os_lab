CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -O2 -Wno-unused-result

all: tcpclient tcpserver udpclient udpserver

tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o $@ $<

tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o $@ $<

udpclient: udpclient.c
	$(CC) $(CFLAGS) -o $@ $<

udpserver: udpserver.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f tcpclient tcpserver udpclient udpserver

.PHONY: all clean