#ifndef CARD_DATA_H
#define CARD_DATA_H

#include "gs.h"
#include "util/gs_idraw.h"

typedef struct {
    int strike; // deal X damage to target
    int heal; // increase target health by X
    int dull; // reduce target attack by X
    int sharpen; // increase target attack by X
    int mass_strike; // deal X damage to all opponent cards
    int mass_heal; // heal all your cards by X
    int mass_dull; // reduce attack of all opponent cards by X
    int mass_sharpen; // increase attack of all your cards by X
    int charge_heal; // increase this cards health by X whenever you play a card
    int charge_sharpen; // increase this cards attack by X whenever you play a card

    bool shield; // when damaged, prevent damage and remove shield
    bool regenerate; // if the card is destroyed, return to hand and remove shield
    bool haste; // attacks early in battle
    bool timebound; // if a cards with timebound is in hand, they must be played before other cards
    bool sacrifice; // when this card is played, you have to select one of your cards to be destroyed
    bool frozen; // can't attack if frozen, gets removed at the first attack attempt
    bool ward; // the first time this card is targeted, ignore the targeted effect
    bool cancel; // remove all abilities from target

    bool bestow_shield; // give shield to target card
    bool bestow_regenerate; // give regenerate to target card
    bool bestow_haste; // give haste to target card
    bool bestow_timebound; // give timebound to target card
    bool bestow_sacrifice; // give sacrifice to target card
    bool bestow_frozen; // give frozen to target card
    bool bestow_ward; // give ward to target card
} card_abilities_t;

typedef struct card_state_t {
    const char *name;
    uint16_t attack;
    uint16_t health;
    bool red;
    bool green;
    bool blue;
    card_abilities_t abilities;
    int current_attack;
    int current_health;
    card_abilities_t current_abilities;

    bool selectable;
    bool hovered;
    gs_vqs transform;

    float anim_duration;
    float lerp;
    gs_vqs prev_transform;
    gs_vqs target_transform;

    uint32_t render_index;
    uint32_t database_index;
} card_state_t;

card_state_t card_new(const char *name, uint16_t attack, uint16_t health, bool red, bool green, bool blue, card_abilities_t abilities);
void card_reset(card_state_t *card);
bool card_has_target_ability(card_abilities_t *abilities);
bool card_has_target_ability_self(card_abilities_t *abilities);
bool card_has_target_ability_other(card_abilities_t *abilities);

card_state_t card_new(const char *name, uint16_t attack, uint16_t health,
    bool red, bool green, bool blue, card_abilities_t abilities) {
    card_state_t card = {0};
    card.name = name;
    card.attack = attack;
    card.health = health;
    card.red = red;
    card.green = green;
    card.blue = blue;
    card.abilities = abilities;

    card_reset(&card);
    card.transform = gs_vqs_default();
    return card;
}

void card_reset(card_state_t *card) {
    card->current_attack = card->attack;
    card->current_health = card->health;
    card->current_abilities = card->abilities;
}

bool card_has_target_ability(card_abilities_t *abilities) {
    return abilities->strike > 0
    || abilities->dull > 0
    || abilities->heal > 0
    || abilities->sharpen > 0
    || abilities->cancel
    || abilities->bestow_shield
    || abilities->bestow_regenerate
    || abilities->bestow_haste
    || abilities->bestow_timebound
    || abilities->bestow_sacrifice
    || abilities->bestow_frozen
    || abilities->bestow_ward;
}

bool card_has_target_ability_self(card_abilities_t *abilities) {
    return abilities->sacrifice;
}

bool card_has_target_ability_other(card_abilities_t *abilities) {
    return false;
}
#endif

