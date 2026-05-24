# Anteater Poker

Anteater Poker is an online multiplayer poker game based on Texas Hold’em with additional UCI Anteater-themed special ability cards. The game is developed in C for the EECS 22L Software Engineering Project.

A central server hosts the poker table, manages the game state, shuffles and deals cards, tracks points, validates player actions, and broadcasts game updates to connected clients. Players use a graphical client application to connect to the server, choose a seat, view their cards, and play against other users or bot players.

## Project Information

- **Project Name:** Anteater Poker
- **Version:** 0.1 Alpha Draft
- **Course:** EECS 22L
- **Institution:** University of California, Irvine
- **Team:** ZotHouse

## Team Members

- Jim Truong
- Hamza Vaid
- Shogo Stuck
- Ben Choi
- Giovanna Dunker Estruquel

## Main Features

- Online multiplayer poker using TCP/IP sockets
- Central server that manages the game state
- Graphical user interface for player clients
- Graphical interface for the server/control-room side
- Texas Hold’em poker rules
- Anteater-themed special ability cards
- Point tracking for each player
- Bot player support for empty seats
- Server-side validation of legal actions
- Scoreboard and end-of-game results
- Unit and system testing support

## Anteater Special Ability Cards

Anteater Poker adds special ability cards that give players unique strategic actions during a round. These cards are separate from regular poker cards unless the specific ability says otherwise.

Example abilities include:

- **Sniff** – The player may view one random private card from a chosen opponent.
- **Ant Trail** – The player may view either the suit or rank of the next community card before it is revealed.
- **Pose** – The player may cancel another player’s special ability card, regardless of whose turn it is.
- **Long Tongue** – The player may swap one private card with the top card of the deck.
- **Wild Grab** – The player may replace one private card with a wildcard that can represent any card needed to form the best possible poker hand.

## Project Structure

Expected project structure:

```text
poker/
├── bin/
│   ├── server
│   └── poker
├── doc/
│   ├── Poker_UserManual.pdf
|   └── Poker_SoftwareSpec.pdf
├── src/
│   ├── server/
│   ├── client/
│   ├── rules/
│   ├── gui/
│   └── bot/
├── include/
├── test/
├── Makefile
├── README.md
├── INSTALL.md
└── COPYRIGHT.md