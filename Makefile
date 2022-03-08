CC=gcc
CFLAGS=-std=gnu99 -Wall -Werror -Wextra -pedantic -pthread -lrt

proj2: proj2.c
	$(CC) $(CFLAGS) -o proj2 proj2.c
