SRCS = server.c cards.c players.c ../protocol/protocol.c

DEBUG = -g

CC = gcc 

all: server
	@echo "Ejecute ./server"

server: $(SRCS)
	$(CC) -o server $(SRCS)

debug: $(SRCS)
	$(CC) $(DEBUG) -o server $(SRCS)

clean:
	rm -f server
