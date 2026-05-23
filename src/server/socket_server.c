/*
 * socket_server.c
 *
 * TCP/IP socket helper functions for the Anteater Poker server.
 *
 * Responsibilities:
 *   - Create and configure the listening server socket.
 *   - Accept client connections.
 *   - Send one message to one client.
 *   - Broadcast a message to multiple clients.
 *   - Parse raw text client messages into NetworkMessage structures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "socket_server.h"

/*
 * init_server
 *
 * Creates a TCP server socket, binds it to the given port, and starts
 * listening for incoming clients.
 *
 * Parameters:
 *   port - TCP port number, usually 10010-10019 for this project
 *
 * Returns:
 *   listening socket file descriptor on success
 *   -1 on failure
 */
int init_server(int port)
{
    int server_fd;
    int opt = 1;
    struct sockaddr_in address;

    /*
     * Create an IPv4 TCP socket.
     *
     * AF_INET      = IPv4
     * SOCK_STREAM  = TCP
     * 0            = default protocol for TCP
     */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket failed");
        return -1;
    }

    /*
     * Allow the port to be reused shortly after the program exits.
     * This prevents "Address already in use" errors during testing.
     */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }

    /*
     * Fill the server address structure.
     *
     * INADDR_ANY means the server accepts connections on any network
     * interface for this machine.
     */
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    /* htons converts the port from host byte order to network byte order. */
    address.sin_port = htons((unsigned short)port);

    /*
     * Bind the socket to the selected IP/port.
     */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }

    /*
     * Put the socket into listening mode so clients can connect.
     */
    if (listen(server_fd, SERVER_BACKLOG) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

/*
 * accept_client
 *
 * Accepts one pending client connection from the listening server socket.
 *
 * Parameters:
 *   server_fd - listening socket descriptor
 *
 * Returns:
 *   new client socket descriptor on success
 *   -1 on failure
 */
int accept_client(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    /*
     * accept creates a new socket specifically for this client.
     * The original server_fd remains open to accept more clients.
     */
    int client_fd = accept(
        server_fd,
        (struct sockaddr *)&client_address,
        &client_len
    );

    if (client_fd < 0) {
        perror("accept failed");
        return -1;
    }

    /*
     * Print the client's IP address and port.
     * inet_ntoa converts the binary IP address into text form.
     * ntohs converts the port from network order to host order.
     */
    printf("New client connected from %s:%d\n",
           inet_ntoa(client_address.sin_addr),
           ntohs(client_address.sin_port));

    return client_fd;
}

/*
 * send_message
 *
 * Sends a null-terminated string to one client socket.
 *
 * Parameters:
 *   socket_fd - client socket descriptor
 *   msg       - message to send
 *
 * Returns:
 *   number of bytes sent, or -1 on failure
 */
int send_message(int socket_fd, const char *msg)
{
    if (socket_fd < 0 || msg == NULL) {
        return -1;
    }

    ssize_t sent = send(socket_fd, msg, strlen(msg), 0);

    if (sent < 0) {
        perror("send failed");
        return -1;
    }

    return (int)sent;
}

/*
 * broadcast_game
 *
 * Sends the same message to every connected client in the fds array.
 *
 * Parameters:
 *   fds         - array of client socket descriptors
 *   num_clients - number of entries in fds
 *   msg         - message to send
 */
void broadcast_game(int fds[], int num_clients, const char *msg)
{
    if (fds == NULL || msg == NULL) {
        return;
    }

    for (int i = 0; i < num_clients; i++) {
        if (fds[i] >= 0) {
            send_message(fds[i], msg);
        }
    }
}

/*
 * parse_client_msg
 *
 * Converts a raw text message into a NetworkMessage struct.
 *
 * Expected message format:
 *   COMMAND:PLAYER_ID:PAYLOAD
 *
 * Examples:
 *   LOGIN:-1:Hamza
 *   SEAT:-1:2
 *   ACTN:0:CHECK
 *   RAISE:0:50
 *
 * Returns:
 *   NetworkMessage with command, player_id, and payload fields filled in.
 */
NetworkMessage parse_client_msg(const char *buf)
{
    NetworkMessage message;

    /* Start with safe default values. */
    memset(&message, 0, sizeof(message));
    snprintf(message.command, COMMAND_LEN, "UNKNOWN");
    message.player_id = -1;
    message.payload[0] = '\0';

    if (buf == NULL) {
        return message;
    }

    /*
     * strtok modifies the string it tokenizes, so copy buf first.
     * This preserves the original received buffer.
     */
    char copy[MESSAGE_BUFFER_SIZE];
    snprintf(copy, sizeof(copy), "%s", buf);

    /*
     * Split the message at ':' characters.
     * The payload reads until newline so it can contain spaces if needed.
     */
    char *command = strtok(copy, ":");
    char *player_id = strtok(NULL, ":");
    char *payload = strtok(NULL, "\n");

    if (command != NULL) {
        snprintf(message.command, COMMAND_LEN, "%s", command);
    }

    if (player_id != NULL) {
        message.player_id = atoi(player_id);
    }

    if (payload != NULL) {
        snprintf(message.payload, PAYLOAD_LEN, "%s", payload);
    }

    return message;
}
