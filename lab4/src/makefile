CC=gcc
CFLAGS=-I. -Wall

.PHONY: all clean

all: parallel_min_max process_memory

parallel_min_max: parallel_min_max.c find_min_max.o utils.o
	$(CC) -o parallel_min_max parallel_min_max.c find_min_max.o utils.o $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) -c find_min_max.c -o find_min_max.o $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c -o utils.o $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

clean:
	rm -f *.o parallel_min_max process_memory