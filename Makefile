CC=gcc
CFLAGS=-Wall

all: bin/client bin/server

bin/client: Client/client2.c
	$(CC) $(CFLAGS) -o bin/client Client/client2.c

bin/server: Serveur/server.c
	$(CC) $(CFLAGS) -o bin/server Serveur/server.c

clean:
	rm -f bin/client bin/server

.PHONY: all clean
