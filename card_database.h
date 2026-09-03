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
    // red common cards
    card_database_add(card_new("2 3", 2, 2, true, false, false, (card_abilities_t){0}));
    card_database_add(card_new("Strike 1 Haste", 1, 1, true, false, false, (card_abilities_t){.strike=1, .haste=true}));
    card_database_add(card_new("1 2 Sharp 2", 1, 2, true, false, false, (card_abilities_t){.sharpen=1}));
    card_database_add(card_new("2 1 Haste", 2, 1, true, false, false, (card_abilities_t){.haste=true}));
    card_database_add(card_new("1 2 Strike 1", 1, 2, true, false, false, (card_abilities_t){.strike=1}));
    card_database_add(card_new("1 4", 1, 3, true, false, false, (card_abilities_t){0}));
    card_database_add(card_new("1 1 Mass Sharp 1", 1, 3, true, false, false, (card_abilities_t){.mass_sharpen=1}));
    card_database_add(card_new("4 1", 3, 1, true, false, false, (card_abilities_t){0}));
    card_database_add(card_new("1 1 Strike 2", 1, 1, true, false, false, (card_abilities_t){.strike=2}));

    // card_database_add(card_new("Blockheart", 2, 2, false, true, false, (card_abilities_t){.heal=1}));
    // card_database_add(card_new("Squareback", 2, 3, false, true, false, (card_abilities_t){0}));
    // card_database_add(card_new("Boxling", 2, 1, false, true, false, (card_abilities_t){.regenerate=1}));
    // card_database_add(card_new("Bullwark", 1, 4, false, true, false, (card_abilities_t){.sacrifice=true}));
    // card_database_add(card_new("Cubeface", 3, 3, false, true, false, (card_abilities_t){0}));
    // card_database_add(card_new("Quadlet", 1, 3, false, true, false, (card_abilities_t){.mass_heal=1}));
    //
    // card_database_add(card_new("Orbling", 2, 2, false, false, true, (card_abilities_t){.dull=1}));
    // card_database_add(card_new("Loopwing", 4, 2, false, false, true, (card_abilities_t){.timebound=true}));
    // card_database_add(card_new("Circlet", 2, 3, false, false, true, (card_abilities_t){0}));
    // card_database_add(card_new("Ringform", 1, 3, false, false, true, (card_abilities_t){.mass_dull=1}));
    // card_database_add(card_new("Roundshell", 1, 4, false, false, true, (card_abilities_t){0}));
    // card_database_add(card_new("Spheric", 3, 2, false, false, true, (card_abilities_t){0}));

    // card_database_add(card_new("Sunfish", 5, 5, false, false, true, (card_abilities_t){0}));
    // card_database_add(card_new("Immortal Jelly", 2, 1, false, false, true, (card_abilities_t){.regenerate=3}));
    // card_database_add(card_new("Kraken", 4, 4, false, false, true, (card_abilities_t){.channel=2}));
}

card_state_t card_get_random(uint32_t render_index) {
    int random_index = (rand() % gs_dyn_array_size(card_database));
    card_state_t card = card_database[random_index];
    card.render_index = render_index;
    return card;
}

#endif
