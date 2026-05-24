#include <stdio.h>
#include <string.h>
#include "cards.h"

Card create_card(Rank rank, Suit suit)
{
    Card card;
    card.rank = rank;
    card.suit = suit;
    return card;
}

int is_valid_card(Card card)
{
    if (card.rank < RANK_TWO || card.rank > RANK_ACE) {
        return 0;
    }

    if (card.suit < SUIT_HEARTS || card.suit > SUIT_SPADES) {
        return 0;
    }

    return 1;
}

int cards_equal(Card a, Card b)
{
    return a.rank == b.rank && a.suit == b.suit;
}

const char *suit_to_string(Suit suit)
{
    switch (suit) {
        case SUIT_HEARTS:
            return "hearts";
        case SUIT_DIAMONDS:
            return "diamonds";
        case SUIT_CLUBS:
            return "clubs";
        case SUIT_SPADES:
            return "spades";
        default:
            return "invalid";
    }
}

const char *rank_to_string(Rank rank)
{
    switch (rank) {
        case RANK_TWO:   return "2";
        case RANK_THREE: return "3";
        case RANK_FOUR:  return "4";
        case RANK_FIVE:  return "5";
        case RANK_SIX:   return "6";
        case RANK_SEVEN: return "7";
        case RANK_EIGHT: return "8";
        case RANK_NINE:  return "9";
        case RANK_TEN:   return "10";
        case RANK_JACK:  return "jack";
        case RANK_QUEEN: return "queen";
        case RANK_KING:  return "king";
        case RANK_ACE:   return "ace";
        default:         return "invalid";
    }
}
void card_to_string(Card card, char *buffer, int buffer_size)
{
    if (buffer == NULL || buffer_size <= 0) {
        return;
    }

    if (!is_valid_card(card)) {
        snprintf(buffer, buffer_size, "invalid_card");
        return;
    }
 
    snprintf(
        buffer,
        buffer_size,
        "%s_of_%s",
        rank_to_string(card.rank),
        suit_to_string(card.suit)
    );
}

void print_card(Card card)
{
    char buffer[64];

    card_to_string(card, buffer, sizeof(buffer));
    printf("%s\n", buffer);
}
