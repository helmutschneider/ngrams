CC=gcc
CFLAGS=-O2 -Wall -Werror -Wpedantic -march=native
LDFLAGS=-lsqlite3

all: clean
	$(CC) $(CFLAGS) app.c -o app $(LDFLAGS)

debug: clean
	$(CC) $(CFLAGS) -DDEBUG app.c -o app $(LDFLAGS)

clean:
	rm -rf app tests

tests: clean
	$(CC) tests.c -o tests $(LDFLAGS) && ./tests

