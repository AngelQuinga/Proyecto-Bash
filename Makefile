CC = gcc
CFLAGS = -Wall

all: QuingaA-bash

Quinga-bash: QuingaA-bash.c
	$(CC) $(CFLAGS) -o QuingaA-bash QuingaA-bash.c

clean:
	rm -f QuingaA-bash
