CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
OBJS = main.o archive.o util.o

.PHONY: all clean

all: tarsau

tarsau: $(OBJS)
	$(CC) $(CFLAGS) -o tarsau $(OBJS)

main.o: main.c tarsau.h
	$(CC) $(CFLAGS) -c main.c

archive.o: archive.c tarsau.h
	$(CC) $(CFLAGS) -c archive.c

util.o: util.c tarsau.h
	$(CC) $(CFLAGS) -c util.c

clean:
	rm -f tarsau $(OBJS)
