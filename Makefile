# Makefile for Anteater Poker

CC = gcc

# GTK settings for Ubuntu Linux.
# Needed because server.c / GUI files use #include <gtk/gtk.h>
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)

CFLAGS = -Wall -Wextra -std=c11 -Iinclude $(GTK_CFLAGS)
LDFLAGS = $(GTK_LIBS)

# Directories
SRC_DIR = src
RULES_DIR = $(SRC_DIR)/rules
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client
GUI_DIR = $(SRC_DIR)/gui
TEST_DIR = test
BIN_DIR = bin
BUILD_DIR = build

# Output programs
SERVER_BIN = $(BIN_DIR)/server
CLIENT_BIN = $(BIN_DIR)/poker
TEST_DECK_BIN = $(BIN_DIR)/test_deck
TEST_SERVER_CLIENT_BIN = $(BIN_DIR)/test_server_client

# Shared rules source files
RULES_OBJ = \
	$(BUILD_DIR)/cards.o \
	$(BUILD_DIR)/deck.o

# Server source files
SERVER_OBJ = \
	$(BUILD_DIR)/server.o \
	$(BUILD_DIR)/game_state.o \
	$(BUILD_DIR)/socket_server.o \
	$(BUILD_DIR)/server_gui.o

# Client source files
CLIENT_OBJ = \
	$(BUILD_DIR)/poker.o \
	$(BUILD_DIR)/client_state.o \
	$(BUILD_DIR)/socket_client.o \
	$(BUILD_DIR)/client_gui.o

.PHONY: all server client test clean directories

all: directories $(SERVER_BIN) $(CLIENT_BIN) $(TEST_DECK_BIN) $(TEST_SERVER_CLIENT_BIN)

directories:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BUILD_DIR)

server: directories $(SERVER_BIN)

client: directories $(CLIENT_BIN)

# Compile files in src/rules/
$(BUILD_DIR)/%.o: $(RULES_DIR)/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

# Compile files in src/server/
$(BUILD_DIR)/%.o: $(SERVER_DIR)/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

# Compile files in src/client/
$(BUILD_DIR)/%.o: $(CLIENT_DIR)/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

# Compile files in src/gui/
$(BUILD_DIR)/%.o: $(GUI_DIR)/%.c | directories
	$(CC) $(CFLAGS) -c $< -o $@

# Build server executable.
$(SERVER_BIN): $(SERVER_OBJ) $(RULES_OBJ) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build client executable.
$(CLIENT_BIN): $(CLIENT_OBJ) $(RULES_OBJ) $(BUILD_DIR)/game_state.o | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build deck test.
$(TEST_DECK_BIN): $(TEST_DIR)/test_deck.c $(RULES_OBJ) | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build server-client test.
$(TEST_SERVER_CLIENT_BIN): $(TEST_DIR)/test_server_client.c | directories
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: all
	./$(TEST_DECK_BIN)
	@echo ""
	@echo "Deck test finished."
	@echo ""
	@echo "To test the server manually:"
	@echo "  Terminal 1: ./bin/server --port 10010 --table \"ZotHouse\""
	@echo "  Terminal 2: ./bin/test_server_client"
	@echo ""
	@echo "To test the terminal client manually:"
	@echo "  Terminal 1: ./bin/server --port 10010 --table \"ZotHouse\""
	@echo "  Terminal 2: ./bin/poker --host localhost --port 10010 --name Hamza"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(SERVER_BIN)
	rm -f $(CLIENT_BIN)
	rm -f $(TEST_DECK_BIN)
	rm -f $(TEST_SERVER_CLIENT_BIN)