RM     = rm -f
CC     = gcc
CFLAGS = -Os -s

default: all

all: pack
	./pack > ../../arm/shell/logo.h

pack: pack.c
	$(CC) $(CFLAGS) -o pack pack.c

.PHONY: clean
clean:
	$(RM) pack
