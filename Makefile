CC=gcc
CFLAGS=-O2 -Wall -Werror -Wpedantic -march=native
LDFLAGS=-lsqlite3 -lreadline

all: clean
	$(CC) $(CFLAGS) $(LDFLAGS) app.c -o app

debug: clean
	$(CC) $(CFLAGS) $(LDFLAGS) -DDEBUG app.c -o app

clean:
	rm -rf app
