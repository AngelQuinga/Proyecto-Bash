CC = gcc
CFLAGS = -Wall

all: Quinga-bash

Quinga-bash: Quinga-bash.c
	$(CC) $(CFLAGS) -o Quinga-bash Quinga-bash.c

clean:
	rm -f Quinga-bash
