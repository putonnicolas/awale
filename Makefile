CC=gcc
CFLAGS=-Wall

all: bin/client bin/server

bin/client: Client/client.c
	$(CC) $(CFLAGS) -o bin/client Client/client.c

bin/server: Serveur/server.c
	$(CC) $(CFLAGS) -o bin/server Serveur/server.c

clean:
	rm -f bin/client bin/server

.PHONY: all clean
