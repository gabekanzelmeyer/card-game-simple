#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "util_card.h"
#include "util_game.h"

#define HOVER_SCREEN_SIZE 300
#define HAND_SPACING 1.5f
#define HAND_FAN_ANGLE 2.0f
#define HAND_CURVE_AMOUNT 0.02f
#define HAND_Y_POSITION_OFFSET 2.6f

enum card_game_phase {
    INIT,
    PLAYER_SELECT_CARD_TO_PLAY,
    OPPONENT_SELECT_CARD_TO_PLAY,
    ANIMATE_PLAYING_CARDS,
    TRIGGER_ON_PLAY_EFFECTS,
    PLAYER_SELECT_TARGET,
    OPPONENT_SELECT_TARGET,
    ANIMATE_TARGET_EFFECT,
    BATTLE
};

static const char* get_phase_name(enum card_game_phase phase) {
    switch (phase) {
        case INIT: return "INIT";
        case PLAYER_SELECT_CARD_TO_PLAY: return "PLAYER_SELECT_CARD_TO_PLAY";
        case OPPONENT_SELECT_CARD_TO_PLAY: return "OPPONENT_SELECT_CARD_TO_PLAY";
        case ANIMATE_PLAYING_CARDS: return "ANIMATE_PLAYING_CARDS";
        case TRIGGER_ON_PLAY_EFFECTS: return "TRIGGER_ON_PLAY_EFFECTS";
        case PLAYER_SELECT_TARGET: return "PLAYER_SELECT_TARGET";
        case OPPONENT_SELECT_TARGET: return "OPPONENT_SELECT_TARGET";
        case ANIMATE_TARGET_EFFECT: return "ANIMATE_TARGET_EFFECT";
        case BATTLE: return "BATTLE";
        default: return "UNKNOWN_PHASE";
    }
}

typedef struct {
    enum card_game_phase phase;
    float phase_timer_prev;
    float phase_timer;
    int phase_tick;
    float game_timer;
    gs_dyn_array(card_state_t) player_hand;
    gs_dyn_array(card_state_t) opponent_hand;
    card_state_t player_card_in_play;
    card_state_t opponent_card_in_play;
    bool player_just_played_card;
    bool opponent_just_played_card;
    bool player_selecting_target;
    bool opponent_selecting_target;
    bool player_card_attacking;
    bool opponent_card_attacking;
    int hovered_index; // -1 means nothing, 0-5 = player hand, 10-15 = opponent hand, 20 = player in play card, 21 = oppoent in play card
    bool visual_update;
} card_game_state_t;

static void position_hand_cards(card_game_state_t *card_game, card_state_t *cards);
static void highlight_remove_all(card_game_state_t *card_game,  game_state_t *game_state);
static void highlight_playable_cards(card_game_state_t *card_game,  game_state_t *game_state);
static void highlight_targetable_cards(card_game_state_t *card_game,  game_state_t *game_state);
static void set_phase(card_game_state_t *card_game, enum card_game_phase phase);
static void set_card_animation(card_state_t *card, gs_vqs_t target_transform, float duration);
static void update_input_indices(card_game_state_t *card_game, game_state_t *game_state);
static void update_card_visuals(card_game_state_t *card_game, game_state_t *game_state);
static void update_card_animations(card_game_state_t *card_game, float dt);
static void trigger_on_play_effects(card_game_state_t *card_game, game_state_t *game_state);
static void trigger_target_effects(card_state_t *source, card_state_t *target);
static void damage_card(card_state_t *source, card_state_t *target, int damage);
static void resolve_damage(card_game_state_t *card_game, game_state_t *game_state);

void card_game_init(card_game_state_t *card_game, game_state_t *game_state) {
    gs_dyn_array_free(card_game->player_hand);
    gs_dyn_array_free(card_game->opponent_hand);

    int render_index = 0;
    for (int i = 0; i < 6; i++) {
        gs_dyn_array_push(card_game->player_hand, card_get_random(render_index++));
        gs_dyn_array_push(card_game->opponent_hand, card_get_random(render_index++));
    }

    card_game->player_card_in_play = (card_state_t){0};
    card_game->opponent_card_in_play = (card_state_t){0};

    card_game->phase = INIT;

    update_card_visuals(card_game, game_state);
}

void card_game_update(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->phase == INIT) {
        position_hand_cards(card_game, card_game->player_hand);
        position_hand_cards(card_game, card_game->opponent_hand);
        set_phase(card_game, PLAYER_SELECT_CARD_TO_PLAY);

    } if (card_game->phase == PLAYER_SELECT_CARD_TO_PLAY) {
        highlight_remove_all(card_game, game_state);
        highlight_playable_cards(card_game, game_state);

        if (card_game->hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) { // will always be an index in players hand here
            highlight_remove_all(card_game, game_state); // remove all highlights

            card_game->player_card_in_play = card_game->player_hand[card_game->hovered_index];
            card_game->player_just_played_card = true;
            gs_dyn_array_erase(card_game->player_hand, card_game->hovered_index);

            // if the opponent doesn't have a card in play, transition
            // to that phase, otherwise start animating playing cards
            if (card_game->opponent_card_in_play.name == NULL) {
                set_phase(card_game, OPPONENT_SELECT_CARD_TO_PLAY);
            } else {
                set_phase(card_game, ANIMATE_PLAYING_CARDS);
            }
        }
    } else if (card_game->phase == OPPONENT_SELECT_CARD_TO_PLAY) {
        // if there are timebound hands in the opponents hand, one of them must be played
        gs_dyn_array(int) timebound_indices = NULL;
        for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
            if (card_game->opponent_hand[i].abilities.timebound) gs_dyn_array_push(timebound_indices, i);
        }
        // if there are timebound cards, grab a random one of those
        int random_index;
        if (gs_dyn_array_size(timebound_indices) > 0) {
            random_index = timebound_indices[(rand() % gs_dyn_array_size(timebound_indices))];
        } else {
            random_index = (rand() % gs_dyn_array_size(card_game->opponent_hand));
        }

        card_game->opponent_card_in_play = card_game->opponent_hand[random_index];
        card_game->opponent_just_played_card = true;

        gs_dyn_array_erase(card_game->opponent_hand, random_index);

        set_phase(card_game, ANIMATE_PLAYING_CARDS);
    } else if (card_game->phase == ANIMATE_PLAYING_CARDS) {
        // if player just played a card, animate it's position into play'
        if (card_game->phase_tick == 1) { // first call in this phase
            if (card_game->player_just_played_card) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(-2., 0.f, 0.f);
                set_card_animation(&card_game->player_card_in_play, target_transform, 0.1f);
                position_hand_cards(card_game, card_game->player_hand);
            }
            if (card_game->opponent_just_played_card) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(2., 0.f, 0.f);
                set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.1f);
                position_hand_cards(card_game, card_game->opponent_hand);
            }
        }

        if (card_game->phase_timer > 0.3) { // let animations finish before changing phase
            set_phase(card_game, TRIGGER_ON_PLAY_EFFECTS);
        }

    } else if (card_game->phase == TRIGGER_ON_PLAY_EFFECTS) {
        if (card_game->player_just_played_card || card_game->opponent_just_played_card) {
            if (card_game->phase_tick == 1) {
                trigger_on_play_effects(card_game, game_state);
                resolve_damage(card_game, game_state);
                position_hand_cards(card_game, card_game->player_hand);
                position_hand_cards(card_game, card_game->opponent_hand);
                card_game->visual_update = true; // on play effects will very likely cause a visual update, so just always do it
            }
            if (card_game->phase_timer > 0.3) { // let animations finish before changing phase
                set_phase(card_game, PLAYER_SELECT_TARGET);
            }
        } else { // if no card was played, immediately transition
            set_phase(card_game, PLAYER_SELECT_TARGET);
        }
    } else if (card_game->phase == PLAYER_SELECT_TARGET) {
        if (card_game->player_just_played_card) {
            if (card_has_target_ability(&card_game->player_card_in_play)) {
                card_game->player_selecting_target = true;
            }
            card_game->player_just_played_card = false;
        }

        if (card_game->player_selecting_target) { // wait while player is selecting target
            highlight_remove_all(card_game, game_state);
            highlight_targetable_cards(card_game, game_state);
            if (card_game->hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
                card_state_t *target;
                if (card_game->hovered_index < 10) {
                    target = &card_game->player_hand[card_game->hovered_index];
                } else if (card_game->hovered_index < 20) {
                    target = &card_game->opponent_hand[card_game->hovered_index - 10];
                } else if (card_game->hovered_index == 20) {
                    target = &card_game->player_card_in_play;
                } else {
                    target = &card_game->opponent_card_in_play;
                }
                trigger_target_effects(&card_game->player_card_in_play, target);
                resolve_damage(card_game, game_state);
                position_hand_cards(card_game, card_game->player_hand);
                position_hand_cards(card_game, card_game->opponent_hand);
                card_game->visual_update = true;
                card_game->player_selecting_target = false;
            }
        } else {
            highlight_remove_all(card_game, game_state);
            set_phase(card_game, OPPONENT_SELECT_TARGET);
        }
    } else if (card_game->phase == OPPONENT_SELECT_TARGET) {
        if (card_game->opponent_just_played_card) {
            card_game->opponent_just_played_card = false;
        }
        set_phase(card_game, BATTLE);
    } else if (card_game->phase == BATTLE) {

        float haste_begin_attack_time = 0.3;
        float haste_end_attack_time = 0.5;
        float haste_resolve_time = 0.7;
        float begin_attack_time = 1.0;
        float end_attack_time = 1.2;
        float attack_resolve_time = 1.5;

        if (card_game->phase_timer_prev < haste_begin_attack_time && card_game->phase_timer > haste_begin_attack_time) {
            printf("haste begin: %f\n", card_game->phase_timer);
            card_game->player_card_attacking = card_game->player_card_in_play.abilities.haste;
            card_game->opponent_card_attacking = card_game->opponent_card_in_play.abilities.haste;
            if (card_game->player_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(-1.5f, 0.f, 0.f);
                set_card_animation(&card_game->player_card_in_play, target_transform, 0.07f);
            }
            if (card_game->opponent_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(1.5f, 0.f, 0.f);
                set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.07f);
            }
        }
        if (card_game->phase_timer_prev < haste_end_attack_time && card_game->phase_timer > haste_end_attack_time) {
            printf("haste end: %f\n", card_game->phase_timer);
            if (card_game->player_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(-2.0f, 0.f, 0.f);
                set_card_animation(&card_game->player_card_in_play, target_transform, 0.1f);
            }
            if (card_game->opponent_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(2.0f, 0.f, 0.f);
                set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.1f);
            }
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            if (card_game->player_card_in_play.abilities.haste) {
                damage_card(&card_game->player_card_in_play, &card_game->opponent_card_in_play, card_game->player_card_in_play.current_attack);
                card_game->visual_update = true;
            }
            if (card_game->opponent_card_in_play.abilities.haste) {
                damage_card(&card_game->opponent_card_in_play, &card_game->player_card_in_play, card_game->opponent_card_in_play.current_attack);
                card_game->visual_update = true;
            }
        }
        if (card_game->phase_timer_prev < haste_resolve_time && card_game->phase_timer > haste_resolve_time) {
            printf("haste resolve: %f\n", card_game->phase_timer);
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            resolve_damage(card_game, game_state);
            position_hand_cards(card_game, card_game->player_hand);
            position_hand_cards(card_game, card_game->opponent_hand);
        }

        // normal animation timer ticks
        if (card_game->phase_timer_prev < begin_attack_time && card_game->phase_timer > begin_attack_time) {
            printf("attack begin: %f\n", card_game->phase_timer);
            card_game->player_card_attacking = !card_game->player_card_in_play.abilities.haste;
            card_game->opponent_card_attacking = !card_game->opponent_card_in_play.abilities.haste;
            if (card_game->player_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(-1.5f, 0.f, 0.f);
                set_card_animation(&card_game->player_card_in_play, target_transform, 0.07f);
            }
            if (card_game->opponent_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(1.5f, 0.f, 0.f);
                set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.07f);
            }
        }
        if (card_game->phase_timer_prev < end_attack_time && card_game->phase_timer > end_attack_time) {
            printf("attack end: %f\n", card_game->phase_timer);
            if (card_game->player_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(-2.0f, 0.f, 0.f);
                set_card_animation(&card_game->player_card_in_play, target_transform, 0.1f);
            }
            if (card_game->opponent_card_attacking) {
                gs_vqs_t target_transform = gs_vqs_default();
                target_transform.position = gs_v3(2.0f, 0.f, 0.f);
                set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.1f);
            }
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            if (!card_game->player_card_in_play.abilities.haste) {
                damage_card(&card_game->player_card_in_play, &card_game->opponent_card_in_play, card_game->player_card_in_play.current_attack);
                card_game->visual_update = true;
            }
            if (!card_game->opponent_card_in_play.abilities.haste) {
                damage_card(&card_game->opponent_card_in_play, &card_game->player_card_in_play, card_game->opponent_card_in_play.current_attack);
                card_game->visual_update = true;
            }
        }
        // normal battle tick
        if (card_game->phase_timer_prev < attack_resolve_time && card_game->phase_timer > attack_resolve_time) {
            printf("attack resolve: %f\n", card_game->phase_timer);
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            resolve_damage(card_game, game_state);
            position_hand_cards(card_game, card_game->player_hand);
            position_hand_cards(card_game, card_game->opponent_hand);
            // if resolving damage doesn't result in a card being destroyed, re-set the phase to battle to reset the phase timer
            if (card_game->player_card_in_play.name != NULL && card_game->opponent_card_in_play.name != NULL) {
                set_phase(card_game, BATTLE);
            }
        }
    }

    float dt = gs_platform_delta_time();
    card_game->phase_timer_prev = card_game->phase_timer;
    card_game->phase_timer += dt;
    card_game->game_timer += dt;
    card_game->phase_tick++;

    if (card_game->visual_update) {
        update_card_visuals(card_game, game_state);
    }
    update_input_indices(card_game, game_state);
    update_card_animations(card_game, dt);

    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&game_state->camera, (int32_t)fbw, (int32_t)fbh);
    // NOTE: cards are drawn depth wise in order, so sort based on draw order before rendering
    // In that same spirit, we should iterate backwards over the cards to check if any are hovered
    cards_render_instanced(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), &game_state->command_buffer, view_projection);
    cards_render_instanced(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), &game_state->command_buffer, view_projection);
    // if there is a player card in play, position and rotate correctly
    if (card_game->player_card_in_play.name != NULL) {
        cards_render_instanced(&card_game->player_card_in_play, 1, &game_state->command_buffer, view_projection);
    }
    // if there is a opponent card in play, position and rotate correctly
    if (card_game->opponent_card_in_play.name != NULL) {
        cards_render_instanced(&card_game->opponent_card_in_play, 1, &game_state->command_buffer, view_projection);
    }
}

static void set_phase(card_game_state_t *card_game, enum card_game_phase phase) {
    card_game->phase = phase;
    card_game->phase_timer = 0.0f;
    card_game->phase_tick = 0;
    printf("SET PHASE: %s TIME: %f\n", get_phase_name(phase), card_game->game_timer);
}

static void position_hand_cards(card_game_state_t *card_game, card_state_t *cards) {
    float spacing, fan_angle, curve_amount, y_offset;
    spacing = HAND_SPACING;
    if (cards == card_game->player_hand) {
        fan_angle = HAND_FAN_ANGLE;
        curve_amount = -HAND_CURVE_AMOUNT;
        y_offset = -HAND_Y_POSITION_OFFSET;
    } else {
        fan_angle = -HAND_FAN_ANGLE;
        curve_amount = HAND_CURVE_AMOUNT;
        y_offset = HAND_Y_POSITION_OFFSET;
    }

    int count = gs_dyn_array_size(cards);
    float start_x = -spacing * (float)count / 2.f + spacing / 2.f;
    float start_tilt = fan_angle * (float)count / 2.f - fan_angle / 2.f;
    for (int i = 0; i < count; i++) {
        gs_vqs_t target = gs_vqs_default();
        target.position.x = start_x + i * spacing;
        target.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * curve_amount * (1.f / spacing) + y_offset;
        target.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * fan_angle)), gs_v3(0, 0, 1));
        set_card_animation(&cards[i], target, 0.2f);
    }
}

static void update_card_visuals(card_game_state_t *card_game, game_state_t *game_state) {
    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); ++i) {
        card_update_visuals(&card_game->player_hand[i], &game_state->immediate_draw);
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); ++i) {
        card_update_visuals(&card_game->opponent_hand[i], &game_state->immediate_draw);
    }
    if (card_game->player_card_in_play.name != NULL) {
        card_update_visuals(&card_game->player_card_in_play, &game_state->immediate_draw);
    }
    if (card_game->opponent_card_in_play.name != NULL) {
        card_update_visuals(&card_game->opponent_card_in_play, &game_state->immediate_draw);
    }
}

void resolve_damage(card_game_state_t *card_game, game_state_t *game_state) {
    bool was_a_card_destroyed = false;
    if (card_game->player_card_in_play.current_health <= 0) {
        if (card_game->player_card_in_play.abilities.regenerate > 0) {
            int regen = --card_game->player_card_in_play.abilities.regenerate;
            card_reset_stats(&card_game->player_card_in_play);
            card_game->player_card_in_play.abilities.regenerate = regen;
            gs_dyn_array_push(card_game->player_hand, card_game->player_card_in_play);
        }
        card_game->player_card_in_play = (card_state_t){0};
        was_a_card_destroyed = true;
    }
    if (card_game->opponent_card_in_play.current_health <= 0) {
        if (card_game->opponent_card_in_play.abilities.regenerate > 0) {
            int regen = --card_game->opponent_card_in_play.abilities.regenerate;
            card_reset_stats(&card_game->opponent_card_in_play);
            card_game->opponent_card_in_play.abilities.regenerate = regen;
            gs_dyn_array_push(card_game->opponent_hand, card_game->opponent_card_in_play);
        }
        card_game->opponent_card_in_play = (card_state_t){0};
        was_a_card_destroyed = true;
    }

    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
        if (card_game->player_hand[i].current_health <= 0) {
            gs_dyn_array_erase(card_game->player_hand, i);
            was_a_card_destroyed = true;
            // decrement i because if multiple cards are deleted we need the
            // index to stay on the current index to correctly remove future cards
            i--;
        }
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
        if (card_game->opponent_hand[i].current_health <= 0) {
            gs_dyn_array_erase(card_game->opponent_hand, i);
            was_a_card_destroyed = true;
            // decrement i because if multiple cards are deleted we need the
            // index to stay on the current index to correctly remove future cards
            i--;
        }
    }

    if (was_a_card_destroyed) {
        if (gs_dyn_array_size(card_game->opponent_hand) == 0
            && gs_dyn_array_size(card_game->player_hand) == 0
            && card_game->player_card_in_play.name == NULL
            && card_game->opponent_card_in_play.name == NULL) {
            printf("DRAW\n");
            game_state->mode = MENU;
        } else if (gs_dyn_array_size(card_game->opponent_hand) == 0
            && card_game->opponent_card_in_play.name == NULL) {
            printf("PLAYER WINS\n");
            game_state->mode = MENU;
        } else if (gs_dyn_array_size(card_game->player_hand) == 0
            && card_game->player_card_in_play.name == NULL) {
            printf("OPPONENT WINS\n");
            game_state->mode = MENU;
        } else {
            if (card_game->player_card_in_play.name == NULL) {
                set_phase(card_game, PLAYER_SELECT_CARD_TO_PLAY);
            } else if (card_game->opponent_card_in_play.name == NULL) {
                set_phase(card_game, OPPONENT_SELECT_CARD_TO_PLAY);
            }
        }
    }
}

void trigger_target_effects(card_state_t *source, card_state_t *target) {
    if (source->abilities.strike > 0) {
        damage_card(source, target, source->abilities.strike);
    }
    if (source->abilities.blunt > 0) {
        target->current_attack = fmax(1, target->current_attack - source->abilities.blunt);
    }
    if (source->abilities.heal > 0) {
        target->current_health += source->abilities.heal;
    }
    if (source->abilities.sharpen > 0) {
        target->current_attack += source->abilities.sharpen;
    }
}

void trigger_on_play_effects(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->player_just_played_card) {
        if (card_game->player_card_in_play.abilities.disarm > 0) {
            card_game->opponent_card_in_play.current_attack = fmax(1, card_game->opponent_card_in_play.current_attack - card_game->player_card_in_play.abilities.disarm);
        }
        if (card_game->player_card_in_play.abilities.warcry > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_attack += card_game->player_card_in_play.abilities.warcry;
            }
        }
        if (card_game->player_card_in_play.abilities.wellspring > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_health += card_game->player_card_in_play.abilities.wellspring;
            }
        }
        if (card_game->player_card_in_play.abilities.swipe > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->opponent_hand[i].current_health -= card_game->player_card_in_play.abilities.swipe;
            }
        }
        if (card_game->player_card_in_play.abilities.cripple > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                card_game->opponent_hand[i].current_attack = fmax(1, card_game->opponent_hand[i].current_attack - card_game->player_card_in_play.abilities.cripple);
            }
        }
        if (card_game->player_card_in_play.abilities.channel > 0) {
            // increase this cards stats
            card_game->player_card_in_play.current_attack += card_game->player_card_in_play.abilities.channel;
            card_game->player_card_in_play.current_health += card_game->player_card_in_play.abilities.channel;
            // increase opponent card in play if colors match
            if (card_do_colors_match(card_game->player_card_in_play, card_game->opponent_card_in_play)) {
                card_game->opponent_card_in_play.current_attack += card_game->player_card_in_play.abilities.channel;
                card_game->opponent_card_in_play.current_health += card_game->player_card_in_play.abilities.channel;
            }
            // increase all hand cards if colors match
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                if (card_do_colors_match(card_game->player_card_in_play, card_game->player_hand[i])) {
                    card_game->player_hand[i].current_attack += card_game->player_card_in_play.abilities.channel;
                    card_game->player_hand[i].current_health += card_game->player_card_in_play.abilities.channel;
                }
            }
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                if (card_do_colors_match(card_game->player_card_in_play, card_game->opponent_hand[i])) {
                    card_game->opponent_hand[i].current_attack += card_game->player_card_in_play.abilities.channel;
                    card_game->opponent_hand[i].current_health += card_game->player_card_in_play.abilities.channel;
                }
            }
        }
        if (card_game->player_card_in_play.abilities.sacrifice > 0 && gs_dyn_array_size(card_game->player_hand)> 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->player_hand));
            card_game->player_hand[random_index].current_health = 0;
        }
        if (card_game->player_card_in_play.abilities.assassinate > 0 && gs_dyn_array_size(card_game->opponent_hand)> 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->opponent_hand));
            card_game->opponent_hand[random_index].current_health = 0;
        }
    }
    if (card_game->opponent_just_played_card) {
        if (card_game->opponent_card_in_play.abilities.disarm > 0) {
            card_game->player_card_in_play.current_attack = fmax(1, card_game->player_card_in_play.current_attack - card_game->opponent_card_in_play.abilities.disarm);
        }
        if (card_game->opponent_card_in_play.abilities.warcry > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                card_game->opponent_hand[i].current_attack += card_game->opponent_card_in_play.abilities.warcry;
            }
        }
        if (card_game->opponent_card_in_play.abilities.wellspring > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                card_game->opponent_hand[i].current_health += card_game->opponent_card_in_play.abilities.wellspring;
            }
        }
        if (card_game->opponent_card_in_play.abilities.swipe > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_health -= card_game->opponent_card_in_play.abilities.swipe;
            }
        }
        if (card_game->opponent_card_in_play.abilities.cripple > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_attack = fmax(1, card_game->player_hand[i].current_attack - card_game->opponent_card_in_play.abilities.cripple);
            }
        }
        if (card_game->opponent_card_in_play.abilities.channel > 0) {
            // increase this cards stats
            card_game->opponent_card_in_play.current_attack += card_game->opponent_card_in_play.abilities.channel;
            card_game->opponent_card_in_play.current_health += card_game->opponent_card_in_play.abilities.channel;
            // increase opponent card in play if colors match
            if (card_do_colors_match(card_game->opponent_card_in_play, card_game->player_card_in_play)) {
                card_game->player_card_in_play.current_attack += card_game->opponent_card_in_play.abilities.channel;
                card_game->player_card_in_play.current_health += card_game->opponent_card_in_play.abilities.channel;
            }
            // increase all hand card stats if colors match
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                if (card_do_colors_match(card_game->opponent_card_in_play, card_game->player_hand[i])) {
                    card_game->player_hand[i].current_attack += card_game->opponent_card_in_play.abilities.channel;
                    card_game->player_hand[i].current_health += card_game->opponent_card_in_play.abilities.channel;
                }
            }
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                if (card_do_colors_match(card_game->opponent_card_in_play, card_game->opponent_hand[i])) {
                    card_game->opponent_hand[i].current_attack += card_game->opponent_card_in_play.abilities.channel;
                    card_game->opponent_hand[i].current_health += card_game->opponent_card_in_play.abilities.channel;
                }
            }
        }
        if (card_game->opponent_card_in_play.abilities.sacrifice > 0 && gs_dyn_array_size(card_game->opponent_hand) > 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->opponent_hand));
            card_game->opponent_hand[random_index].current_health = 0;
        }
        if (card_game->opponent_card_in_play.abilities.assassinate > 0 && gs_dyn_array_size(card_game->player_hand) > 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->player_hand));
            card_game->opponent_hand[random_index].current_health = 0;
        }
    }
}

static void damage_card(card_state_t *source, card_state_t *target, int damage) {
    if (target->current_abilities.shield > 0) {
        target->current_abilities.shield--;
    } else {
        target->current_health -= damage;
    }
}

static void card_game_lerp_card_transform(card_state_t *card, float dt) {
    card->transform.position.x = gs_interp_smoothstep(card->prev_transform.position.x,
                         card->target_transform.position.x,
                         card->lerp);
    card->transform.position.y = gs_interp_smoothstep(card->prev_transform.position.y,
                         card->target_transform.position.y,
                         card->lerp);
    card->transform.position.z = gs_interp_smoothstep(card->prev_transform.position.z,
                         card->target_transform.position.z,
                         card->lerp);

    card->transform.scale.x = gs_interp_smoothstep(card->prev_transform.scale.x,
                         card->target_transform.scale.x,
                         card->lerp);
    card->transform.scale.y = gs_interp_smoothstep(card->prev_transform.scale.y,
                         card->target_transform.scale.y,
                         card->lerp);
    card->transform.scale.z = gs_interp_smoothstep(card->prev_transform.scale.z,
                         card->target_transform.scale.z,
                         card->lerp);

    card->transform.rotation.x = gs_interp_smoothstep(card->prev_transform.rotation.x,
                         card->target_transform.rotation.x,
                         card->lerp);
    card->transform.rotation.y = gs_interp_smoothstep(card->prev_transform.rotation.y,
                         card->target_transform.rotation.y,
                         card->lerp);
    card->transform.rotation.z = gs_interp_smoothstep(card->prev_transform.rotation.z,
                         card->target_transform.rotation.z,
                         card->lerp);
    card->transform.rotation.w = gs_interp_smoothstep(card->prev_transform.rotation.w,
                         card->target_transform.rotation.w,
                         card->lerp);
}

static void set_card_animation(card_state_t *card, gs_vqs_t target_transform, float duration) {
    card->prev_transform.position = card->transform.position;
    card->prev_transform.rotation = card->transform.rotation;
    card->prev_transform.scale = card->transform.scale;
    card->target_transform.position = target_transform.position;
    card->target_transform.rotation = target_transform.rotation;
    card->target_transform.scale = target_transform.scale;
    card->anim_duration = duration;
    card->lerp = 0.0f;
}

static void update_card_animations(card_game_state_t *card_game, float dt) {
    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
        card_game->player_hand[i].lerp += dt / card_game->player_hand[i].anim_duration;
        if (card_game->player_hand[i].lerp >= 1) {
            card_game->player_hand[i].lerp = 1;
        }
        card_game_lerp_card_transform(&card_game->player_hand[i], dt);
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
        card_game->opponent_hand[i].lerp += dt / card_game->opponent_hand[i].anim_duration;
        if (card_game->opponent_hand[i].lerp >= 1) {
            card_game->opponent_hand[i].lerp = 1;
        }
        card_game_lerp_card_transform(&card_game->opponent_hand[i], dt);
    }
    if (card_game->player_card_in_play.name != NULL) {
        card_game->player_card_in_play.lerp += dt / card_game->player_card_in_play.anim_duration;
        if (card_game->player_card_in_play.lerp >= 1) {
            card_game->player_card_in_play.lerp = 1;
        }
        card_game_lerp_card_transform(&card_game->player_card_in_play, dt);
    }
    if (card_game->opponent_card_in_play.name != NULL) {
        card_game->opponent_card_in_play.lerp += dt / card_game->opponent_card_in_play.anim_duration;
        if (card_game->opponent_card_in_play.lerp >= 1) {
            card_game->opponent_card_in_play.lerp = 1;
        }
        card_game_lerp_card_transform(&card_game->opponent_card_in_play, dt);
    }
}

static void highlight_remove_all(card_game_state_t *card_game,  game_state_t *game_state) {
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (card_game->player_hand[i].selectable) {
            card_game->player_hand[i].selectable = false;
            card_game->visual_update = true;
        }
    }
    for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
        if (card_game->opponent_hand[i].selectable) {
            card_game->opponent_hand[i].selectable = false;
            card_game->visual_update = true;
        }
    }
    if (card_game->player_card_in_play.name != NULL && card_game->player_card_in_play.selectable) {
        card_game->player_card_in_play.selectable = false;
        card_game->visual_update = true;
    }
    if (card_game->opponent_card_in_play.name != NULL && card_game->opponent_card_in_play.selectable) {
        card_game->opponent_card_in_play.selectable = false;
        card_game->visual_update = true;
    }
}

static void highlight_playable_cards(card_game_state_t *card_game,  game_state_t *game_state) {
    bool has_timebound = false;
    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
        if (card_game->player_hand[i].abilities.timebound) has_timebound = true;
    }

    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (!has_timebound || card_game->player_hand[i].abilities.timebound) {
            if (!card_game->player_hand[i].selectable) {
                card_game->player_hand[i].selectable = true;
                card_game->visual_update = true;
            }
        } else {
            if (card_game->player_hand[i].selectable) {
                card_game->player_hand[i].selectable = false;
                card_game->visual_update = true;
            }
        }
    }
}

static void highlight_targetable_cards(card_game_state_t *card_game,  game_state_t *game_state) {
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (!card_game->player_hand[i].selectable) {
            card_game->player_hand[i].selectable = true;
            card_game->visual_update = true;
        }
    }
    for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
        if (!card_game->opponent_hand[i].selectable) {
            card_game->opponent_hand[i].selectable = true;
            card_game->visual_update = true;
        }
    }
    if (card_game->player_card_in_play.name != NULL && !card_game->player_card_in_play.selectable) {
        card_game->player_card_in_play.selectable = true;
        card_game->visual_update = true;
    }
    if (card_game->opponent_card_in_play.name != NULL && !card_game->opponent_card_in_play.selectable) {
        card_game->opponent_card_in_play.selectable = true;
        card_game->visual_update = true;
    }
}

static void card_game_set_player_targets_selectable(card_game_state_t *card_game,  game_state_t *game_state) {
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (!card_game->player_hand[i].selectable) {
            card_game->player_hand[i].selectable = true;
            card_game->visual_update = true;
        }
    }
    for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
        if (!card_game->opponent_hand[i].selectable) {
            card_game->opponent_hand[i].selectable = true;
            card_game->visual_update = true;
        }
    }
    if (card_game->player_card_in_play.name != NULL && !card_game->player_card_in_play.selectable) {
        card_game->player_card_in_play.selectable = true;
        card_game->visual_update = true;
    }
    if (card_game->opponent_card_in_play.name != NULL && !card_game->opponent_card_in_play.selectable) {
        card_game->opponent_card_in_play.selectable = true;
        card_game->visual_update = true;
    }
}

static void update_input_indices(card_game_state_t *card_game, game_state_t *game_state) {
    gs_vec2 mouse_pos = gs_platform_mouse_positionv();
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&game_state->camera, (int32_t)fbw, (int32_t)fbh);
    gs_vec2 screen_pos;

    // increase scale of hovered hand card and keep track of which card for later,
    // also, if the card can be selected, outline it in green
    int hovered_index = -1;
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        world_to_screen(card_game->player_hand[i].transform.position, &screen_pos, view_projection, fbw, fbh);
        if (card_game->player_hand[i].selectable && gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE) {
            if (!card_game->player_hand[i].hovered) {
                card_game->player_hand[i].hovered = true;
                gs_vqs_t target = card_game->player_hand[i].target_transform;
                target.scale = gs_v3(1.2f, 1.2f, 1.2f);
                set_card_animation(&card_game->player_hand[i], target, 0.1f);
            }
            hovered_index = i;
            break;
        }
    }
    for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
        world_to_screen(card_game->opponent_hand[i].transform.position, &screen_pos, view_projection, fbw, fbh);
        if (card_game->opponent_hand[i].selectable && gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE) {
            if (!card_game->opponent_hand[i].hovered) {
                card_game->opponent_hand[i].hovered = true;
                gs_vqs_t target = card_game->opponent_hand[i].target_transform;
                target.scale = gs_v3(1.2f, 1.2f, 1.2f);
                set_card_animation(&card_game->opponent_hand[i], target, 0.1f);
            }
            hovered_index = i + 10; // opponent hand hovered indices is 10-15
            break;
        }
    }
    if (card_game->player_card_in_play.name != NULL) {
        world_to_screen(card_game->player_card_in_play.transform.position, &screen_pos, view_projection, fbw, fbh);
        if (card_game->player_card_in_play.selectable && gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE) {
            if (!card_game->player_card_in_play.hovered) {
                card_game->player_card_in_play.hovered = true;
                gs_vqs_t target = card_game->player_card_in_play.target_transform;
                target.scale = gs_v3(1.2f, 1.2f, 1.2f);
                set_card_animation(&card_game->player_card_in_play, target, 0.1f);
            }
            hovered_index = 20; // 20 is player in play card
        }
    }
    if (card_game->opponent_card_in_play.name != NULL) {
        world_to_screen(card_game->opponent_card_in_play.transform.position, &screen_pos, view_projection, fbw, fbh);
        if (card_game->opponent_card_in_play.selectable && gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE) {
            if (!card_game->opponent_card_in_play.hovered) {
                card_game->opponent_card_in_play.hovered = true;
                gs_vqs_t target = card_game->opponent_card_in_play.target_transform;
                target.scale = gs_v3(1.2f, 1.2f, 1.2f);
                set_card_animation(&card_game->opponent_card_in_play, target, 0.1f);
            }
            hovered_index = 21; // 21 is opponent in play card
        }
    }

    card_game->hovered_index = hovered_index;

    // change scale of cards that aren't hovered
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (card_game->player_hand[i].hovered && i != hovered_index) {
            card_game->player_hand[i].hovered = false;
            gs_vqs_t target = card_game->player_hand[i].target_transform;
            target.scale = gs_v3(1.0f, 1.0f, 1.0f);
            set_card_animation(&card_game->player_hand[i], target, 0.1f);
        }
    }
    for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
        if (card_game->opponent_hand[i].hovered && (i + 10) != hovered_index) {
            card_game->opponent_hand[i].hovered = false;
            gs_vqs_t target = card_game->opponent_hand[i].target_transform;
            target.scale = gs_v3(1.0f, 1.0f, 1.0f);
            set_card_animation(&card_game->opponent_hand[i], target, 0.1f);
        }
    }
    if (card_game->player_card_in_play.hovered && 20 != hovered_index) {
        card_game->player_card_in_play.hovered = false;
        gs_vqs_t target = card_game->player_card_in_play.target_transform;
        target.scale = gs_v3(1.0f, 1.0f, 1.0f);
        set_card_animation(&card_game->player_card_in_play, target, 0.1f);
    }
    if (card_game->opponent_card_in_play.hovered && 21 != hovered_index) {
        card_game->opponent_card_in_play.hovered = false;
        gs_vqs_t target = card_game->opponent_card_in_play.target_transform;
        target.scale = gs_v3(1.0f, 1.0f, 1.0f);
        set_card_animation(&card_game->opponent_card_in_play, target, 0.1f);
    }
}
