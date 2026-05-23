/*
 * poker.c
 *
 * Terminal-based Anteater Poker client.
 *
 * This file lets a player:
 * - connect to the server
 * - send LOGIN
 * - send actions such as START, CHECK, CALL, FOLD, RAISE, and QUIT
 * - receive and process server messages
 *
 * This version fixes:
 * - STAT messages now update ClientState
 * - HAND messages now update ability/card status
 * - multiple server messages received at once are processed line by line
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_state.h"
#include "socket_client.h"

/* Default server host if the user does not provide --host. */
#define DEFAULT_HOST "localhost"

/* Default project port. */
#define DEFAULT_PORT 10010

/* Maximum size for user input typed into the terminal. */
#define INPUT_SIZE 256

/*
 * Converts a phase string from the server into a GamePhase enum.
 *
 * Example:
 * "PREFLOP" becomes PHASE_PREFLOP.
 */
static GamePhase phase_from_string(const char *phase)
{
    /* If phase is NULL, use LOBBY as a safe default. */
    if (phase == NULL)
    {
        return PHASE_LOBBY;
    }

    /* Compare the server text against each known phase. */
    if (strcmp(phase, "LOBBY") == 0)
    {
        return PHASE_LOBBY;
    }

    if (strcmp(phase, "PREFLOP") == 0)
    {
        return PHASE_PREFLOP;
    }

    if (strcmp(phase, "FLOP") == 0)
    {
        return PHASE_FLOP;
    }

    if (strcmp(phase, "TURN") == 0)
    {
        return PHASE_TURN;
    }

    if (strcmp(phase, "RIVER") == 0)
    {
        return PHASE_RIVER;
    }

    if (strcmp(phase, "SHOWDOWN") == 0)
    {
        return PHASE_SHOWDOWN;
    }

    if (strcmp(phase, "GAME_OVER") == 0)
    {
        return PHASE_GAME_OVER;
    }

    /* If the phase string is unknown, return LOBBY safely. */
    return PHASE_LOBBY;
}

/*
 * Converts an ability string from the server into an AbilityType enum.
 *
 * Example:
 * "SNIFF" becomes ABILITY_SNIFF.
 */
static AbilityType ability_from_string(const char *ability)
{
    /* If ability is NULL, use ABILITY_NONE as a safe default. */
    if (ability == NULL)
    {
        return ABILITY_NONE;
    }

    /* Compare the server text against each known ability. */
    if (strcmp(ability, "NONE") == 0)
    {
        return ABILITY_NONE;
    }

    if (strcmp(ability, "SNIFF") == 0)
    {
        return ABILITY_SNIFF;
    }

    if (strcmp(ability, "ANT_TRAIL") == 0)
    {
        return ABILITY_ANT_TRAIL;
    }

    if (strcmp(ability, "POSE") == 0)
    {
        return ABILITY_POSE;
    }

    if (strcmp(ability, "LONG_TONGUE") == 0)
    {
        return ABILITY_LONG_TONGUE;
    }

    if (strcmp(ability, "WILD_GRAB") == 0)
    {
        return ABILITY_WILD_GRAB;
    }

    /* If the ability string is unknown, return no ability safely. */
    return ABILITY_NONE;
}

/*
 * Parses command-line arguments for the client.
 *
 * Supported options:
 * --host <server_host>
 * --port <port_number>
 * --name <player_name>
 */
static void parse_client_args(
    int argc,
    char *argv[],
    char *host,
    int host_size,
    int *port,
    char *name,
    int name_size)
{
    /* Set default host, name, and port first. */
    snprintf(host, host_size, "%s", DEFAULT_HOST);
    snprintf(name, name_size, "Player");
    *port = DEFAULT_PORT;

    /* Walk through every command-line argument. */
    for (int i = 1; i < argc; i++)
    {
        /* Read the server host. */
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc)
        {
            snprintf(host, host_size, "%s", argv[++i]);
        }

        /* Read the server port. */
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
        {
            *port = atoi(argv[++i]);
        }

        /* Read the player display name. */
        else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc)
        {
            snprintf(name, name_size, "%s", argv[++i]);
        }
    }
}

/*
 * Parses a STAT message from the server and updates ClientState.
 *
 * Expected format:
 * STAT:-1:phase=PREFLOP;players=1;pot=100;turn=0;community=0
 */
static void parse_stat_message(ClientState *client, const char *message)
{
    /* Buffer for the phase text. */
    char phase_text[64];

    /* Variables for values sent by the server. */
    int players = 0;
    int pot = 0;
    int turn = -1;
    int community = 0;

    /* Make sure inputs are valid. */
    if (client == NULL || message == NULL)
    {
        return;
    }

    /*
     * Read the values from the STAT message.
     *
     * %63[^;] reads the phase text until the semicolon.
     */
    int matched = sscanf(
        message,
        "STAT:-1:phase=%63[^;];players=%d;pot=%d;turn=%d;community=%d",
        phase_text,
        &players,
        &pot,
        &turn,
        &community);

    /*
     * If sscanf found all five expected values, update ClientState.
     */
    if (matched == 5)
    {
        client->phase = phase_from_string(phase_text);
        client->player_count = players;
        client->pot = pot;
        client->current_turn = turn;
        client->community_count = community;
        set_client_status(client, "Received public game state.");
    }
    else
    {
        set_client_status(client, "Could not parse game state.");
    }
}

/*
 * Parses a HAND message from the server and updates ClientState.
 *
 * Expected format:
 * HAND:0:Jack of Diamonds,Ace of Clubs,ability=SNIFF
 *
 * This starter parser stores the ability.
 * It prints the cards, but does not fully convert card strings back into Card structs yet.
 */
static void parse_hand_message(ClientState *client, const char *message)
{
    /* Stores the seat number from the HAND message. */
    int seat = -1;

    /* Stores the two card strings as text. */
    char card1_text[128];
    char card2_text[128];

    /* Stores the ability string from the server. */
    char ability_text[64];

    /* Make sure inputs are valid. */
    if (client == NULL || message == NULL)
    {
        return;
    }

    /*
     * Parse the HAND message.
     *
     * %127[^,] reads everything until the first comma.
     * %127[^,] reads everything until the second comma.
     * %63s reads the ability text after ability=.
     */
    int matched = sscanf(
        message,
        "HAND:%d:%127[^,],%127[^,],ability=%63s",
        &seat,
        card1_text,
        card2_text,
        ability_text);

    /*
     * If parsing worked, update the client's ability and status.
     */
    if (matched == 4)
    {
        client->seat = seat;
        client->ability = ability_from_string(ability_text);

        printf("Private card 1: %s\n", card1_text);
        printf("Private card 2: %s\n", card2_text);
        printf("Ability card: %s\n", ability_text);

        set_client_status(client, "Received private hand.");
    }
    else
    {
        set_client_status(client, "Could not parse private hand.");
    }
}

/*
 * Handles one complete server message line.
 *
 * Example lines:
 * INFO:-1:Connected. Send LOGIN:-1:your_name
 * SEAT:0:Welcome Hamza
 * STAT:-1:phase=PREFLOP;players=1;pot=0;turn=0;community=0
 * HAND:0:Jack of Diamonds,Ace of Clubs,ability=SNIFF
 * OK:-1:Check accepted
 * ERROR:-1:Not your turn
 */
static void handle_single_server_message(ClientState *client, const char *message)
{
    /* Make sure inputs are valid. */
    if (client == NULL || message == NULL || message[0] == '\0')
    {
        return;
    }

    /* Print the raw server message for debugging. */
    printf("Server says: %s\n", message);

    /*
     * SEAT message means the server assigned this client a seat.
     */
    if (strncmp(message, "SEAT:", 5) == 0)
    {
        int seat = -1;

        /* Extract the seat number. */
        if (sscanf(message, "SEAT:%d:", &seat) == 1)
        {
            client->seat = seat;
            set_client_status(client, "Logged in and seated.");
        }
    }

    /*
     * STAT message contains public game state.
     */
    else if (strncmp(message, "STAT:", 5) == 0)
    {
        parse_stat_message(client, message);
    }

    /*
     * HAND message contains the private hand and ability.
     */
    else if (strncmp(message, "HAND:", 5) == 0)
    {
        parse_hand_message(client, message);
    }

    /*
     * ERROR message means the server rejected an action.
     */
    else if (strncmp(message, "ERROR:", 6) == 0)
    {
        set_client_status(client, "Server returned an error.");
    }

    /*
     * OK message means the server accepted an action.
     */
    else if (strncmp(message, "OK:", 3) == 0)
    {
        set_client_status(client, "Action accepted.");
    }

    /*
     * INFO message is just informational.
     */
    else if (strncmp(message, "INFO:", 5) == 0)
    {
        set_client_status(client, "Server sent information.");
    }
}

/*
 * Handles a server buffer that may contain multiple messages.
 *
 * TCP can combine messages together.
 * For example, one read may contain:
 *
 * STAT:-1:phase=PREFLOP;players=1;pot=0;turn=0;community=0
 * HAND:0:Jack of Diamonds,Ace of Clubs,ability=SNIFF
 *
 * This function splits the buffer by newline and handles each message separately.
 */
static void handle_server_buffer(ClientState *client, const char *buffer)
{
    /* Local copy because strtok modifies the string. */
    char copy[CLIENT_BUFFER_SIZE];

    /* Make sure inputs are valid. */
    if (client == NULL || buffer == NULL)
    {
        return;
    }

    /* Copy the server buffer safely. */
    snprintf(copy, sizeof(copy), "%s", buffer);

    /* Split the buffer into lines using newline as the delimiter. */
    char *line = strtok(copy, "\n");

    /* Process every complete line. */
    while (line != NULL)
    {
        handle_single_server_message(client, line);
        line = strtok(NULL, "\n");
    }
}

/*
 * Receives one buffer from the server and processes all messages inside it.
 *
 * Returns:
 * 1 if messages were received
 * 0 if server disconnected
 * -1 if an error occurred
 */
static int receive_and_handle_server_messages(ClientState *client)
{
    /* Buffer for raw server data. */
    char buffer[CLIENT_BUFFER_SIZE];

    /* Read data from server. */
    int bytes_read = receive_from_server(client->socket_fd, buffer, sizeof(buffer));

    /* If server sent data, handle it. */
    if (bytes_read > 0)
    {
        handle_server_buffer(client, buffer);
        return 1;
    }

    /* If bytes_read is 0, the server disconnected. */
    if (bytes_read == 0)
    {
        printf("Server disconnected.\n");
        return 0;
    }

    /* Otherwise, an error occurred. */
    return -1;
}

/*
 * Prints the available terminal commands.
 */
static void print_help(void)
{
    printf("\nCommands:\n");
    printf("  start              Start a new hand\n");
    printf("  check              Send CHECK action\n");
    printf("  call               Send CALL action\n");
    printf("  fold               Send FOLD action\n");
    printf("  raise <amount>     Send RAISE action\n");
    printf("  state              Print local client state\n");
    printf("  help               Show commands\n");
    printf("  quit               Disconnect\n\n");
}

/*
 * Converts user input into a message that follows the server protocol.
 */
static void build_action_message(
    const ClientState *client,
    const char *input,
    char *message,
    int message_size)
{
    /* Make sure inputs are valid. */
    if (client == NULL || input == NULL || message == NULL)
    {
        return;
    }

    /* Clear output message first. */
    message[0] = '\0';

    /* Start a new hand. */
    if (strcmp(input, "start") == 0)
    {
        snprintf(message, message_size, "START:%d:\n", client->seat);
    }

    /* Send CHECK action. */
    else if (strcmp(input, "check") == 0)
    {
        snprintf(message, message_size, "ACTN:%d:CHECK\n", client->seat);
    }

    /* Send CALL action. */
    else if (strcmp(input, "call") == 0)
    {
        snprintf(message, message_size, "ACTN:%d:CALL\n", client->seat);
    }

    /* Send FOLD action. */
    else if (strcmp(input, "fold") == 0)
    {
        snprintf(message, message_size, "ACTN:%d:FOLD\n", client->seat);
    }

    /* Send RAISE action. */
    else if (strncmp(input, "raise", 5) == 0)
    {
        int amount = 0;

        /* Extract the raise amount. */
        if (sscanf(input, "raise %d", &amount) == 1)
        {
            snprintf(message, message_size, "RAISE:%d:%d\n", client->seat, amount);
        }
        else
        {
            printf("Usage: raise <amount>\n");
        }
    }

    /* Send QUIT command. */
    else if (strcmp(input, "quit") == 0)
    {
        snprintf(message, message_size, "QUIT:%d:\n", client->seat);
    }
}

/*
 * Main program entry point.
 */
int main(int argc, char *argv[])
{
    /* Stores local client information. */
    ClientState client;

    /* Server host name or IP address. */
    char host[128];

    /* Player display name. */
    char name[CLIENT_NAME_LEN];

    /* Stores user terminal input. */
    char input[INPUT_SIZE];

    /* Stores outgoing message to server. */
    char outgoing[CLIENT_BUFFER_SIZE];

    /* Server port number. */
    int port;

    /* Initialize local client state. */
    init_client_state(&client);

    /* Parse command-line options. */
    parse_client_args(
        argc,
        argv,
        host,
        sizeof(host),
        &port,
        name,
        sizeof(name));

    /* Save player name in client state. */
    set_client_name(&client, name);

    /* Print connection details. */
    printf("Connecting to server...\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("Name: %s\n", name);

    /* Connect to server. */
    client.socket_fd = connect_to_server(host, port);

    /* Stop if connection fails. */
    if (client.socket_fd < 0)
    {
        fprintf(stderr, "Could not connect to server.\n");
        return 1;
    }

    /* Mark client as connected. */
    client.connected = 1;
    set_client_status(&client, "Connected to server.");

    /*
     * Receive initial INFO message.
     */
    receive_and_handle_server_messages(&client);

    /*
     * Send LOGIN message.
     */
    snprintf(outgoing, sizeof(outgoing), "LOGIN:-1:%s\n", client.player_name);
    send_to_server(client.socket_fd, outgoing);

    /*
     * Receive SEAT response and possibly STAT response.
     */
    receive_and_handle_server_messages(&client);

    /* Show available commands. */
    print_help();

    /*
     * Main input loop.
     */
    while (1)
    {
        /* Print prompt. */
        printf("poker> ");

        /* Read input from user. */
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        /* Remove newline from input. */
        input[strcspn(input, "\n")] = '\0';

        /* Show help command. */
        if (strcmp(input, "help") == 0)
        {
            print_help();
            continue;
        }

        /* Print local client state. */
        if (strcmp(input, "state") == 0)
        {
            print_client_state(&client);
            continue;
        }

        /* Convert input into outgoing server message. */
        build_action_message(&client, input, outgoing, sizeof(outgoing));

        /* If outgoing message is empty, command was unknown. */
        if (outgoing[0] == '\0')
        {
            printf("Unknown command. Type 'help' for options.\n");
            continue;
        }

        /* Send command to server. */
        send_to_server(client.socket_fd, outgoing);

        /* If quitting, stop after sending quit message. */
        if (strcmp(input, "quit") == 0)
        {
            break;
        }

        /*
         * Receive and process server response.
         */
        int result = receive_and_handle_server_messages(&client);

        /* If server disconnected or error happened, stop. */
        if (result <= 0)
        {
            break;
        }
    }

    /* Close socket connection. */
    close(client.socket_fd);

    /* Mark client as disconnected. */
    client.connected = 0;

    printf("Disconnected from server.\n");

    return 0;
}