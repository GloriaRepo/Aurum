CFLAGS = -Wall -Wextra -g
LIBS = -lmysqlclient
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)

all: server client client_gui

server: server.c bank.h
	gcc $(CFLAGS) server.c -o server $(LIBS)

client: client.c bank.h
	gcc $(CFLAGS) client.c -o client

client_gui: client_gui.c bank.h
	gcc $(CFLAGS) $(GTK_CFLAGS) client_gui.c -o client_gui $(GTK_LIBS)

clean:
	rm -f server client client_gui
