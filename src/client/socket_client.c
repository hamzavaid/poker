/*
 * socket_client.c
 *
 * This file implements client-side socket communication.
 * It lets the client connect to the poker server, send messages,
 * and receive messages.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "socket_client.h"

/*
 * Connects to the poker server using TCP.
 *
 * This function:
 * 1. Creates a socket.
 * 2. Looks up the server hostname.
 * 3. Builds the server address.
 * 4. Connects to the server.
 * 5. Returns the connected socket.
 */
int connect_to_server(const char *host, int port)
{
    /* Socket file descriptor for the connection. */
    int socket_fd;

    /* Server address structure used by connect(). */
    struct sockaddr_in server_addr;

    /* Host information returned by gethostbyname(). */
    struct hostent *server;

    /* Make sure the host string is valid. */
    if (host == NULL) {
        return -1;
    }

    /*
     * Create a TCP socket.
     *
     * AF_INET means IPv4.
     * SOCK_STREAM means TCP.
     * 0 means use the default protocol for TCP.
     */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Check if socket creation failed. */
    if (socket_fd < 0) {
        perror("socket failed");
        return -1;
    }

    /*
     * Convert the host name into an IP address.
     * Example: "localhost" becomes 127.0.0.1.
     */
    server = gethostbyname(host);

    /* Check if the hostname could not be found. */
    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host: %s\n", host);
        close(socket_fd);
        return -1;
    }

    /* Clear the server address structure before filling it. */
    memset(&server_addr, 0, sizeof(server_addr));

    /* Use IPv4 addresses. */
    server_addr.sin_family = AF_INET;

    /*
     * Copy the resolved server IP address into the sockaddr_in structure.
     *
     * h_addr_list[0] is the first IP address found for the host.
     */
    memcpy(
        &server_addr.sin_addr.s_addr,
        server->h_addr_list[0],
        server->h_length
    );

    /*
     * Convert the port number to network byte order.
     * htons means "host to network short."
     */
    server_addr.sin_port = htons((unsigned short)port);

    /*
     * Try to connect to the server with retries.
     * Retry up to 5 times with 500ms delays so that we can run server and client at same time without worries.
     */
    int max_retries = 5;
    int retry_delay_ms = 500;

    for (int attempt = 0; attempt < max_retries; attempt++) {
        if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
            /* Connection successful. */
            return socket_fd;
        }

        /* Connection failed. Check if we should retry. */
        if (attempt < max_retries - 1) {
            printf("Connection refused. Retrying in %d ms...\n", retry_delay_ms);
            usleep(retry_delay_ms * 1000); /* Convert ms to microseconds. */
        }
    }

    /* All retries exhausted. */
    perror("connect failed after retries");
    close(socket_fd);
    return -1;
}

/*
 * Sends a string message to the server.
 */
int send_to_server(int socket_fd, const char *message)
{
    /* Validate inputs. */
    if (socket_fd < 0 || message == NULL) {
        return -1;
    }

    /*
     * Write the message to the socket.
     * strlen(message) sends only the actual text, not unused buffer space.
     */
    ssize_t sent = write(socket_fd, message, strlen(message));

    /* Check if writing failed. */
    if (sent < 0) {
        perror("write failed");
        return -1;
    }

    /* Return the number of bytes sent. */
    return (int)sent;
}

/*
 * Receives a string message from the server.
 */
int receive_from_server(int socket_fd, char *buffer, int buffer_size)
{
    /* Validate inputs. */
    if (socket_fd < 0 || buffer == NULL || buffer_size <= 0) {
        return -1;
    }

    /*
     * Clear the buffer so it does not contain old data.
     */
    memset(buffer, 0, buffer_size);

    /*
     * Read from the socket.
     * buffer_size - 1 leaves space for the null terminator.
     */
    ssize_t bytes_read = read(socket_fd, buffer, buffer_size - 1);

    /* Check if reading failed. */
    if (bytes_read < 0) {
        perror("read failed");
        return -1;
    }

    /*
     * bytes_read can be:
     * - positive: message received
     * - zero: server disconnected
     * - negative: error, already handled above
     */
    return (int)bytes_read;
}