#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 8
#define MAX_MESSAGE 1024

int read_bytes(int socketID, void *buffer, int total) {
    int bytes;
	int read;

	read = 0;
	bytes = 1;

	while ((read < total) && (bytes > 0)) {
		bytes = recv(socketID, buffer + read, total - read, 0);
		read = read + bytes;
	}

	return (read);
}

int read_message(int socketID, char *message) {
    int bytes;
    uint32_t buffer;

    bytes = read_bytes(socketID, &buffer, BUFFER_SIZE);
    buffer = ntohl(buffer);

    if (bytes != 0) {
        bytes = read_bytes(socketID, message, buffer);
    }

    // printf("read_message -- %s\n", message);

    return (bytes);
}

int send_message(int socketID, char *message) {
    int bytes;
    uint32_t lon;
    uint32_t lon_net;
    char buffer[MAX_MESSAGE];

    // printf("send_message -- %s\n", mens);

    lon = strlen(message) + 1;
    lon_net = htonl(lon);

    memcpy(buffer, &lon_net, BUFFER_SIZE);
    memcpy(buffer + BUFFER_SIZE, message, lon);

    bytes = send(socketID, buffer, (lon + BUFFER_SIZE), 0);

    return (bytes);
}
