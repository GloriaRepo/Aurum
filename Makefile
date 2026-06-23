CFLAGS = -Wall -Wextra -g
LIBS = -lmysqlclient

all: server client

server: server.c bank.h
	gcc $(CFLAGS) server.c -o server $(LIBS)

client: client.c bank.h
	gcc $(CFLAGS) client.c -o client

clean:
	rm -f server client
