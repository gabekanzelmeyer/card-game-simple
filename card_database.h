#ifndef CARD_DATABASE_H
#define CARD_DATABASE_H

#include "gs.h"

#include "card_data.h"

gs_dyn_array(card_state_t) card_database;

static void card_database_add(card_state_t card) {
    card.database_index = gs_dyn_array_size(card_database);
    gs_dyn_array_push(card_database, card);
}

void card_database_init() {
    gs_dyn_array_free(card_database);

    card_database_add(card_new("Red 2/3", 2, 3, true, false, false, COMMON, 1, (card_abilities_t){0}));
    card_database_add(card_new("Red 3/2", 3, 2, true, false, false, COMMON, 1, (card_abilities_t){0}));
    card_database_add(card_new("Red 2/2 Sharpen 2", 2, 2, true, false, false, COMMON, 1, (card_abilities_t){.sharpen=2}));
    card_database_add(card_new("Red 2/2 Strike 2", 2, 2, true, false, false, COMMON, 1, (card_abilities_t){.strike=2}));
    card_database_add(card_new("Red 3/1 Haste", 3, 1, true, false, false, COMMON, 1, (card_abilities_t){.haste=true}));
    card_database_add(card_new("Red 1/2 Mass Sharp 1", 1, 2, true, false, false, COMMON, 1, (card_abilities_t){.mass_sharpen=1}));
    // card_database_add(card_new("Red 1/1 Strike 3", 1, 1, true, false, false, (card_abilities_t){.strike=3}));
    // card_database_add(card_new("Red 1/2 Mass Sharp 1", 1, 2, true, false, false, (card_abilities_t){.mass_sharpen=1}));
    // card_database_add(card_new("Red 1/3 Sharp 3", 1, 3, true, false, false, (card_abilities_t){.sharpen=3}));
    // card_database_add(card_new("Red 2/2 Haste", 2, 2, true, false, false, (card_abilities_t){.haste=true}));
    // card_database_add(card_new("Red 2/3 Sharp 2", 2, 3, true, false, false, (card_abilities_t){.sharpen=2}));
    // card_database_add(card_new("Red 3/1 Strike 2", 3, 1, true, false, false, (card_abilities_t){.strike=2}));

    card_database_add(card_new("Green 3/2", 3, 2, false, true, false, COMMON, 1, (card_abilities_t){0}));
    card_database_add(card_new("Green 2/3", 2, 3, false, true, false, COMMON, 1, (card_abilities_t){0}));
    card_database_add(card_new("Green 2/1 Heal 3", 2, 1, false, true, false, COMMON, 1, (card_abilities_t){.heal=3}));
    card_database_add(card_new("Green 2/2 Shield", 2, 2, false, true, false, COMMON, 1, (card_abilities_t){.shield=true}));
    card_database_add(card_new("Green 3/1 Regenerate", 3, 1, false, true, false, COMMON, 1, (card_abilities_t){.regenerate=true}));
    card_database_add(card_new("Green 1/2 Mass heal 1", 1, 2, false, true, false, COMMON, 1, (card_abilities_t){.mass_heal=1}));
    // card_database_add(card_new("Green 2/2 Regen", 2, 2, false, true, false, (card_abilities_t){.regenerate=true}));
    // card_database_add(card_new("Green 1/1 Charge 1/1", 2, 2, false, true, false, (card_abilities_t){.charge_health=1, .charge_attack=1}));
    // card_database_add(card_new("Green 4/1 Regen", 4, 1, false, true, false, (card_abilities_t){.regenerate=true}));
    // card_database_add(card_new("Green 2/1 Mass Heal 1", 2, 1, false, true, false, (card_abilities_t){.mass_heal=1}));
    // card_database_add(card_new("Green 1/2 Regen Heal 2", 1, 2, false, true, false, (card_abilities_t){.heal=2, .regenerate=true}));
    // card_database_add(card_new("Green 2/2 Evade", 2, 2, false, true, false, (card_abilities_t){.evade=true}));

    card_database_add(card_new("Blue 1/4", 1, 4, false, false, true, COMMON, 1, (card_abilities_t){0}));
    card_database_add(card_new("Blue 2/3", 2, 3, false, false, true, COMMON, 1, (card_abilities_t){0}));
    card_database_add(card_new("Blue 1/3 Dull 2", 1, 3, false, false, true, COMMON, 1, (card_abilities_t){.dull=2}));
    card_database_add(card_new("Blue 3/5 Frozen", 3, 5, false, false, true, COMMON, 1, (card_abilities_t){.frozen=true}));
    card_database_add(card_new("Blue 2/2 Evade", 2, 2, false, false, true, COMMON, 1, (card_abilities_t){.evade=true}));
    card_database_add(card_new("Blue 1/2 Mass Dull 1", 1, 2, false, false, true, COMMON, 1, (card_abilities_t){.mass_dull=1}));
    // card_database_add(card_new("Blue 3/3 Ward", 3, 3, false, false, true, (card_abilities_t){.ward=true}));
    // card_database_add(card_new("Blue 3/2 Evade", 3, 2, false, false, true, (card_abilities_t){.evade=true}));
    // card_database_add(card_new("Blue 5/6 Frozen", 5, 6, false, false, true, (card_abilities_t){.frozen=true}));
    // card_database_add(card_new("Blue 2/2 Cancel", 2, 2, false, false, true, (card_abilities_t){.cancel=true}));
    // card_database_add(card_new("Blue 2/3 Freeze", 2, 3, false, false, true, (card_abilities_t){.bestow_frozen=true}));
    // card_database_add(card_new("Blue 3/2 Freeze", 3, 2, false, false, true, (card_abilities_t){.bestow_frozen=true}));
}

card_state_t card_get_random(bool red, bool green, bool blue) {
    gs_dyn_array(card_state_t) filtered_cards = NULL;
    bool all_disabled = !(red || green || blue);
    for (int i = 0; i < gs_dyn_array_size(card_database); i++) {
        if (card_database[i].red && (red || all_disabled)) gs_dyn_array_push(filtered_cards, card_database[i]);
        else if (card_database[i].green && (green || all_disabled)) gs_dyn_array_push(filtered_cards, card_database[i]);
        else if (card_database[i].blue && (blue || all_disabled))  gs_dyn_array_push(filtered_cards, card_database[i]);
    }
    int random_index = (rand() % gs_dyn_array_size(filtered_cards));
    card_state_t card = filtered_cards[random_index];
    gs_dyn_array_free(filtered_cards);
    return card;
}

bool hand_contains_card(gs_dyn_array(card_state_t) hand, card_state_t card) {
    for (int i = 0; i < gs_dyn_array_size(hand); i++) {
        if (hand[i].database_index == card.database_index) return true;
    }
    return false;
}

gs_dyn_array(card_state_t) hand_get_random(bool red, bool green, bool blue) {
    gs_dyn_array(card_state_t) hand = NULL;
    for (int i = 0; i < 6; i++) {
        card_state_t card = card_get_random(red, green, blue);
        while (hand_contains_card(hand, card)) { // only one of each card can go in a hand
            card = card_get_random(red, green, blue);
        }
        gs_dyn_array_push(hand, card);
    }
    return hand;
}

gs_dyn_array(card_state_t) hand_get_random_color() {
    int color = rand() % 3;
    bool red = color == 0;
    bool green = color == 1;
    bool blue = color == 2;
    return hand_get_random(red, green, blue);
}

#endif
