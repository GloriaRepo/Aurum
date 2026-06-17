CFLAGS = -Wall -Wextra -g

all: server client

server: server.c
	gcc $(CFLAGS) server.c -o server

client: client.c
	gcc $(CFLAGS) client.c -o client

clean:
	rm -f server client
