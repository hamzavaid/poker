/*
 * server.c
 *
 * Main server program for Anteater Poker.
 *
 * Responsibilities:
 *   - Read command-line options such as --port and --table.
 *   - Start the TCP socket server.
 *   - Accept client connections.
 *   - Receive client messages.
 *   - Update GameState by calling game-state helper functions.
 *   - Send public table updates and private hand messages.
 *
 * This is currently a starter server. It supports basic login, hand start,
 * simple actions, raises, disconnects, and public/private messages.
 * Full poker rules and Anteater abilities should later be moved into
 * poker_rules.c and anteater_abilities.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/select.h>

#include "game_state.h"
#include "socket_server.h"

/*
 * parse_server_args
 *
 * Reads command-line arguments and updates the ServerConfig.
 *
 * Supported examples:
 *   ./bin/server --port 10010 --table "ZotHouse"
 *   ./bin/server --points 1000 --bots 1 --log logs/game.log
 *
 * This function is static because it is only used inside server.c.
 */
static void parse_server_args(int argc, char *argv[], ServerConfig *config)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config->port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--table") == 0 && i + 1 < argc) {
            snprintf(config->table_name, MAX_TABLE_NAME_LEN, "%s", argv[++i]);
        } else if (strcmp(argv[i], "--points") == 0 && i + 1 < argc) {
            config->starting_points = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--bots") == 0 && i + 1 < argc) {
            config->bot_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            snprintf(config->log_path, MAX_LOG_PATH_LEN, "%s", argv[++i]);
        }
    }
}

/*
 * collect_client_fds
 *
 * Builds an array of active client socket file descriptors.
 * This is used when broadcasting a message to all connected clients.
 *
 * Parameters:
 *   game        - current server game state
 *   fds         - output array to store socket descriptors
 *   num_clients - output count of connected client sockets
 */
static void collect_client_fds(const GameState *game, int fds[], int *num_clients)
{
    *num_clients = 0;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status != PLAYER_EMPTY &&
            game->players[i].socket_fd >= 0) {
            fds[*num_clients] = game->players[i].socket_fd;
            (*num_clients)++;
        }
    }
}

/*
 * find_seat_by_socket
 *
 * Finds which player seat belongs to a given socket descriptor.
 *
 * Returns:
 *   seat number if found, otherwise -1.
 */
static int find_seat_by_socket(const GameState *game, int socket_fd)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].socket_fd == socket_fd &&
            game->players[i].status != PLAYER_EMPTY) {
            return i;
        }
    }

    return -1;
}

/*
 * send_public_state_to_all
 *
 * Creates a public GAME_STATE/STAT message and broadcasts it to all clients.
 * This message does not include private cards.
 */
static void send_public_state_to_all(const GameState *game)
{
    int fds[MAX_PLAYERS];
    int num_clients = 0;
    char state_msg[MESSAGE_BUFFER_SIZE];

    collect_client_fds(game, fds, &num_clients);
    build_public_game_state(game, state_msg, sizeof(state_msg));
    broadcast_game(fds, num_clients, state_msg);
}

/*
 * send_private_hands
 *
 * Sends each active player their own private hand message.
 * Each private hand must only be sent to that player's socket.
 */
static void send_private_hands(GameState *game)
{
    char hand_msg[MESSAGE_BUFFER_SIZE];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == PLAYER_ACTIVE &&
            game->players[i].socket_fd >= 0) {
            build_private_hand_message(game, i, hand_msg, sizeof(hand_msg));
            send_message(game->players[i].socket_fd, hand_msg);
        }
    }
}

/*
 * handle_client_message
 *
 * Processes one message received from a client.
 *
 * Supported commands:
 *   LOGIN:-1:<name>     - add a player to the table
 *   START:-1:anything   - start a new hand
 *   ACTN:<seat>:CHECK   - check
 *   ACTN:<seat>:CALL    - call
 *   ACTN:<seat>:FOLD    - fold
 *   RAISE:<seat>:50     - raise by 50 points
 *   QUIT:<seat>:bye     - leave table
 *
 * The server remains authoritative. Clients request actions, but the server
 * decides whether to accept or reject them.
 */
static void handle_client_message(GameState *game, int client_fd, const char *buf)
{
    /* Convert the raw text message into a NetworkMessage struct. */
    NetworkMessage msg = parse_client_msg(buf);

    printf("Received: command=%s player=%d payload=%s\n",
           msg.command,
           msg.player_id,
           msg.payload);

    /*
     * LOGIN command:
     * Assigns the client to an empty seat and stores their name.
     */
    if (strcmp(msg.command, "LOGIN") == 0) {
        int seat = add_player(game, client_fd, msg.payload);

        if (seat < 0) {
            send_message(client_fd, "ERROR:-1:Seat unavailable\n");
            return;
        }

        char reply[MESSAGE_BUFFER_SIZE];
        snprintf(reply, sizeof(reply), "SEAT:%d:Welcome %s\n", seat, msg.payload);
        send_message(client_fd, reply);

        /* Let everyone know the table changed. */
        send_public_state_to_all(game);
        return;
    }

    /*
     * START command:
     * Starts a new hand, broadcasts public state, then sends private hands.
     */
    if (strcmp(msg.command, "START") == 0) {
        start_new_hand(game);
        send_public_state_to_all(game);
        send_private_hands(game);
        return;
    }

    /*
     * ACTN command:
     * Handles simple player actions. Current version supports CHECK, CALL,
     * and FOLD. More detailed legality checks should later go in poker_rules.c.
     */
    if (strcmp(msg.command, "ACTN") == 0) {
        int seat = find_seat_by_socket(game, client_fd);

        if (seat < 0) {
            send_message(client_fd, "ERROR:-1:Player not logged in\n");
            return;
        }

        /* Only the current-turn player may act. */
        if (seat != game->current_turn) {
            send_message(client_fd, "ERROR:-1:Not your turn\n");
            return;
        }

        if (strcmp(msg.payload, "FOLD") == 0) {
            game->players[seat].status = PLAYER_FOLDED;
            send_message(client_fd, "OK:-1:Fold accepted\n");
        } else if (strcmp(msg.payload, "CHECK") == 0) {
            send_message(client_fd, "OK:-1:Check accepted\n");
        } else if (strcmp(msg.payload, "CALL") == 0) {
            send_message(client_fd, "OK:-1:Call accepted\n");
        } else {
            send_message(client_fd, "ERROR:-1:Illegal action\n");
            return;
        }

        /* Simple turn advancement for now.
         * Later, the rules module should decide when a betting round ends.
         */
        for (int i = 1; i <= MAX_PLAYERS; i++) {
            int next = (seat + i) % MAX_PLAYERS;

            if (game->players[next].status == PLAYER_ACTIVE) {
                game->current_turn = next;
                break;
            }
        }

        send_public_state_to_all(game);
        return;
    }

    /*
     * RAISE command:
     * Adds points to the pot and updates the player's current bet.
     *
     * Current version only checks positive amount and available points.
     * Later, poker_rules.c should check minimum raise and betting-round rules.
     */
    if (strcmp(msg.command, "RAISE") == 0) {
        int seat = find_seat_by_socket(game, client_fd);
        int amount = atoi(msg.payload);

        if (seat < 0) {
            send_message(client_fd, "ERROR:-1:Player not logged in\n");
            return;
        }

        if (amount <= 0 || amount > game->players[seat].points) {
            send_message(client_fd, "ERROR:-1:Invalid raise amount\n");
            return;
        }

        game->players[seat].points -= amount;
        game->players[seat].current_bet += amount;
        game->pot += amount;
        game->current_bet = game->players[seat].current_bet;

        send_message(client_fd, "OK:-1:Raise accepted\n");
        send_public_state_to_all(game);
        return;
    }

    /*
     * QUIT command:
     * Removes the player from the table and closes their client socket.
     */
    if (strcmp(msg.command, "QUIT") == 0) {
        int seat = find_seat_by_socket(game, client_fd);

        if (seat >= 0) {
            remove_player(game, seat);
        }

        close(client_fd);
        send_public_state_to_all(game);
        return;
    }

    /* If no command matched, reject the message. */
    send_message(client_fd, "ERROR:-1:Unknown command\n");
}

/*
 * main
 *
 * Program entry point for the server.
 *
 * Flow:
 *   1. Load default config.
 *   2. Parse command-line arguments.
 *   3. Open a listening socket.
 *   4. Initialize GameState.
 *   5. Enter a select() loop to handle multiple clients.
 */
int main(int argc, char *argv[])
{
    ServerConfig config;
    GameState game;

    /* Load default options first, then override with command-line values. */
    init_server_config(&config);
    parse_server_args(argc, argv, &config);

    /* Create the server listening socket. */
    int server_fd = init_server(config.port);

    if (server_fd < 0) {
        fprintf(stderr, "Failed to start server.\n");
        return 1;
    }

    /* Initialize the official server-side game state. */
    init_game_state(&game, &config, server_fd);

    printf("Anteater Poker server started.\n");
    printf("Table: %s\n", config.table_name);
    printf("Port: %d\n", config.port);
    printf("Waiting for clients...\n");

    fd_set read_fds;
    int max_fd;

    /*
     * Main server loop.
     *
     * select() lets the server wait for activity on:
     *   - the listening socket, meaning a new client wants to connect
     *   - any connected client socket, meaning a client sent a message
     */
    while (1) {
        FD_ZERO(&read_fds);

        /* Always watch the main listening socket. */
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        /* Watch all connected client sockets. */
        for (int i = 0; i < MAX_PLAYERS; i++) {
            int fd = game.players[i].socket_fd;

            if (game.players[i].status != PLAYER_EMPTY && fd >= 0) {
                FD_SET(fd, &read_fds);

                if (fd > max_fd) {
                    max_fd = fd;
                }
            }
        }

        /* Block until at least one socket becomes ready. */
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (ready < 0) {
            perror("select failed");
            break;
        }

        /*
         * If the listening socket is ready, accept a new client connection.
         */
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept_client(server_fd);

            if (client_fd >= 0) {
                send_message(client_fd, "INFO:-1:Connected. Send LOGIN:-1:your_name\n");
            }
        }

        /*
         * Check each connected client to see if it sent data.
         */
        for (int i = 0; i < MAX_PLAYERS; i++) {
            int client_fd = game.players[i].socket_fd;

            if (game.players[i].status != PLAYER_EMPTY &&
                client_fd >= 0 &&
                FD_ISSET(client_fd, &read_fds)) {
                char buffer[MESSAGE_BUFFER_SIZE];
                memset(buffer, 0, sizeof(buffer));

                /* Read one message from the client. */
                ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

                /*
                 * bytes_read <= 0 means the client disconnected or an error happened.
                 */
                if (bytes_read <= 0) {
                    printf("Client disconnected.\n");
                    close(client_fd);
                    remove_player(&game, i);
                    send_public_state_to_all(&game);
                } else {
                    handle_client_message(&game, client_fd, buffer);
                }
            }
        }
    }

    /* Clean up the listening socket before exiting. */
    close(server_fd);
    return 0;
}
