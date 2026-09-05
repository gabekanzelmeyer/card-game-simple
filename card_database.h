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

    card_database_add(card_new("2 3 Red", 2, 3, true, false, false, (card_abilities_t){0}));
    card_database_add(card_new("1 3 Red", 1, 3, true, false, false, (card_abilities_t){0}));
    card_database_add(card_new("3 1 Red", 3, 1, true, false, false, (card_abilities_t){0}));
    card_database_add(card_new("Strike 1 Haste", 1, 1, true, false, false, (card_abilities_t){.strike=1, .haste=true}));
    card_database_add(card_new("1 2 Sharp 2", 1, 2, true, false, false, (card_abilities_t){.sharpen=1}));
    card_database_add(card_new("2 1 Haste", 2, 1, true, false, false, (card_abilities_t){.haste=true}));
    card_database_add(card_new("1 2 Strike 1", 1, 2, true, false, false, (card_abilities_t){.strike=1}));
    card_database_add(card_new("1 1 Mass Sharp 1", 1, 3, true, false, false, (card_abilities_t){.mass_sharpen=1}));
    card_database_add(card_new("1 1 Strike 2", 1, 1, true, false, false, (card_abilities_t){.strike=2}));

    card_database_add(card_new("3 2 Green", 3, 2, false, true, false, (card_abilities_t){0}));
    card_database_add(card_new("2 3 Green", 2, 3, false, true, false, (card_abilities_t){0}));
    card_database_add(card_new("2 3 Green2", 2, 3, false, true, false, (card_abilities_t){0}));
    card_database_add(card_new("2 2 Heal 1", 2, 2, false, true, false, (card_abilities_t){.heal=1}));
    card_database_add(card_new("1 1 Charge 0/1", 2, 2, false, true, false, (card_abilities_t){.charge_heal=1}));
    card_database_add(card_new("1 1 Heal 3", 3, 2, false, true, false, (card_abilities_t){.heal=3}));
    card_database_add(card_new("2 1 Regenerate", 3, 2, false, true, false, (card_abilities_t){.regenerate=true}));
    card_database_add(card_new("1 1 Regen Heal 1", 1, 1, false, true, false, (card_abilities_t){.heal=1, .regenerate=true}));
    card_database_add(card_new("1 3 Heal 1", 1, 3, false, true, false, (card_abilities_t){.heal=1}));

    card_database_add(card_new("1 4 Blue", 1, 4, false, false, true, (card_abilities_t){0}));
    card_database_add(card_new("2 3 Blue", 2, 3, false, false, true, (card_abilities_t){0}));
    card_database_add(card_new("3 2 Blue", 3, 2, false, false, true, (card_abilities_t){0}));
    card_database_add(card_new("2 2 Dull 1", 2, 2, false, false, true, (card_abilities_t){.dull=1}));
    card_database_add(card_new("1 3 Dull 1", 1, 3, false, false, true, (card_abilities_t){.dull=1}));
    card_database_add(card_new("1 1 Mass Dull", 1, 1, false, false, true, (card_abilities_t){.mass_dull=1}));
    card_database_add(card_new("2 1 Dull Ward", 2, 1, false, false, true, (card_abilities_t){.dull=1, .ward=true}));
    card_database_add(card_new("1 2 Freeze", 1, 2, false, false, true, (card_abilities_t){.bestow_frozen=true}));
    card_database_add(card_new("1 3 Freeze", 1, 3, false, false, true, (card_abilities_t){.bestow_frozen=true}));
}

card_state_t card_get_random() {
    int random_index = (rand() % gs_dyn_array_size(card_database));
    card_state_t card = card_database[random_index];
    return card;
}

gs_dyn_array(card_state_t) hand_get_random() {
    gs_dyn_array(card_state_t) hand = NULL;
    for (int i = 0; i < 6; i++) {
        card_state_t card = card_get_random();
        gs_dyn_array_push(hand, card);
    }
    return hand;
}

#endif
