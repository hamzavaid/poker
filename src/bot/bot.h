#ifndef BOT_H
#define BOT_H

#include "cards.h"

/*
 * The bot's local memory of what is happening.
 */
typedef struct {
    int my_seat;
    int points;
    int current_bet;
    int table_highest_bet;
    int pot;
    char phase[32];
    
    Card my_cards[2];
    Card community_cards[5];
    int community_count;

    int is_my_turn; 
} BotState;


/* Parsing: Updates the BotState when the server broadcasts a message */
void bot_update_state(BotState *state, const char *server_message);

/* AI Logic: Looks at the BotState and returns "CALL", "FOLD", "CHECK", or "RAISE" */
const char* bot_decide_action(const BotState *state);

/* AI Logic: Calculates raise amount if it decides to raise */
int bot_calculate_raise(const BotState *state);

#endif
