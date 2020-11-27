SRCS = client.c ../protocol/protocol.c

DEBUG = -g

CC = gcc

all: client
	@echo "Ejecute: ./client [servidor [puerto]]"

client: $(SRCS)
	$(CC) -o client $(SRCS)

debug: $(SRCS)
	$(CC) $(DEBUG) -o client $(SRCS)

clean:
	rm -f client
