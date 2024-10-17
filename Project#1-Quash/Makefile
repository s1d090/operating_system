#shell2: shell2.c
#	gcc -o shell2 shell2.c -I.

# Makefile for the Quash Shell project

CC = gcc
CFLAGS = -I. -std=c99  # Use C99 standard explicitly

shell: shell.c
	$(CC) $(CFLAGS) -o shell shell.c

clean:
	rm -f shell
