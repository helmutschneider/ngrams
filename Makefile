CC=gcc
CFLAGS=-O2 -Wall -Werror -Wpedantic
LDFLAGS=-lsqlite3

all: clean
	$(CC) $(CFLAGS) $(LDFLAGS) app.c -o app

clean:
	rm -rf app
