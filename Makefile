CC = gcc

CFLAGS = -Wall -Wextra -g -pthread

MONGO_CFLAGS = $(shell pkg-config --cflags libmongoc-1.0)
MONGO_LIBS = $(shell pkg-config --libs libmongoc-1.0)

TARGETS = server client

all: $(TARGETS)

server: server.c
	$(CC) $(MONGO_CFLAGS) server.c -o server $(MONGO_LIBS)

client: client.c
	$(CC) $(CFLAGS) client.c -o client