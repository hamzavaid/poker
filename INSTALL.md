# Installation Guide

This document explains how to install, build, run, and uninstall Anteater Poker.

## System Requirements

Recommended system requirements:

- **Operating System:** Linux, macOS, or Windows with a compatible terminal environment
- **Compiler:** GCC for C language
- **Build Tools:** `make`
- **Archive Tool:** `gtar` or `tar`
- **GUI Library:** GTK 3
- **Memory:** At least 512 MB recommended
- **Storage:** At least 50 MB available space
- **Network:** TCP/IP connection between server and clients

The project is expected to be built and tested on the EECS course Linux servers.

## Installing the Release Package

The release package contains the compiled program files. No compilation is required for this package.

Unpack the release archive:

    gtar xvzf Poker_V1.0.tar.gz

This should create a directory named:

    poker/

Open the user manual:

    evince poker/doc/Poker_UserManual.pdf

Start the server:

    poker/bin/server --port 22022 --table "ZotHouse"

Start the client:

    poker/bin/poker --host server_name_or_ip --port 22022 --name username

Start server and client in same terminal:
    poker/bin/server --port 22022 --table "ZotHouse" & poker/bin/poker --host server_name_or_ip --port 22022
    (This can be repeated for any number of clients by repeating the "& poker/bin/poker..." part)

Replace `server_name_or_ip` with the actual host name or IP address of the server.

Example:

    poker/bin/poker --host crystalcove.eecs.uci.edu --port 22022

## Installing From Source

The source package contains the project source code and must be compiled before running.

Unpack the source archive:

    gtar xvzf Poker_V1.0_src.tar.gz

Go into the project directory:

    cd poker

Compile the program:

    make

Run unit and system tests:

    make test

Clean generated files:

    make clean

## Configuration Options

| Option | Example | Description |
|---|---|---|
| `--host` | `--host crystalcove.eecs.uci.edu` | Server name or IP address used by the client |
| `--port` | `--port 22022` | TCP port used by the server and client |
| `--name` | `--name Hamza` | Optional player display name |
| `--bots` | `--bots 1` | Number of bot players added by the server |
| `--points` | `--points 1000` | Starting points for each player |
| `--log` | `--log logs/game.log` | Location of the game log file |

## Running the Server

The server acts as the dealer and controls the game state. It shuffles the deck, deals cards, tracks points, validates player actions, and broadcasts updates.

Example server command:

    ./bin/server --port 22022 --table "ZotHouse"

## Running the Client

Each player runs a client program to connect to the server.

Example client command:

    ./bin/poker --host server_name_or_ip --port 22022

After connecting, the player enters a display name, chooses an open seat, and waits for the game to begin.

## Running With Bots

The server may add bot players to empty seats.

Example:

    ./bin/server --port 22022 --table "ZotHouse" --bots 1

## Troubleshooting

### Cannot connect to server

Check that:

- The server is running.
- The host name or IP address is correct.
- The port number matches the server port.
- The firewall or network is not blocking the connection.

### Port already in use

Choose a different port number:

    ./bin/server --port 22023 --table "ZotHouse"

Then clients must connect using the same port:

    ./bin/poker --host server_name_or_ip --port 22023

### Build fails

Check that GCC, make, GTK 3, and the required development libraries are installed.

Try cleaning and rebuilding:

    make clean
    make

### GUI does not open

Check that the graphical environment and GTK 3 libraries are available.

## Uninstalling

To uninstall Anteater Poker, delete the project directory:

    rm -rf poker

If you only want to remove compiled files while keeping the source code, run:

    make clean

## Notes

Anteater Poker is intended for educational use only. The game uses points only and must not be used for gambling or real-money betting.