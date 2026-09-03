#ifndef CARD_GAME_H
#define CARD_GAME_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "card_data.h"
#include "card_renderer.h"
#include "card_database.h"
#include "game_util.h"

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
    ANIMATE_TARGET_EFFECTS,
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
        case ANIMATE_TARGET_EFFECTS: return "ANIMATE_TARGET_EFFECTS";
        case BATTLE: return "BATTLE";
        default: return "UNKNOWN_PHASE";
    }
}

enum card_game_target_type {
    NONE,
    ANY,
    SELF,
    OTHER
};

typedef struct {
    int hand_index;
    int target_index;
} ai_selection_t;

typedef struct {
    char name[64];
} stats_key_t;

typedef struct {
    enum card_game_phase phase;
    float phase_timer_prev;
    float phase_timer;
    int phase_tick;
    float game_timer;
    gs_dyn_array(card_state_t) player_hand;
    gs_dyn_array(card_state_t) opponent_hand;
    // cache the initial hands of a game so that it can be used for post game statistics
    gs_dyn_array(card_state_t) player_hand_cache;
    gs_dyn_array(card_state_t) opponent_hand_cache;
    card_state_t player_card_in_play;
    card_state_t opponent_card_in_play;
    bool player_just_played_card;
    bool player_on_play_triggered;
    bool opponent_just_played_card;
    bool opponent_on_play_triggered;
    ai_selection_t opponent_selection;
    enum card_game_target_type player_selecting_target_type;
    enum card_game_target_type opponent_selecting_target_type;
    bool player_card_attacking;
    bool opponent_card_attacking;
    card_state_t *target;
    int hovered_index; // -1 means nothing, 0-5 = player hand, 10-15 = opponent hand, 20 = player in play card, 21 = oppoent in play card
    bool visual_update;
    bool simulate_player;
    int simulation_count;
    int winning_card_counts[100];
    int player_wins, opponent_wins, draws;
    ai_selection_t player_selection;
    float game_speed;
} card_game_state_t;

static void position_hand_cards(card_game_state_t *card_game, card_state_t *cards);
static void highlight_remove_all(card_game_state_t *card_game,  game_state_t *game_state);
static void highlight_playable_cards(card_game_state_t *card_game,  game_state_t *game_state);
static void highlight_targetable_cards(card_game_state_t *card_game,  game_state_t *game_state, enum card_game_target_type type);
static void set_phase(card_game_state_t *card_game, enum card_game_phase phase);
static void set_card_animation(card_state_t *card, gs_vqs_t target_transform, float duration);
static void update_input_indices(card_game_state_t *card_game, game_state_t *game_state);
static void update_card_visuals(card_game_state_t *card_game, game_state_t *game_state);
static void update_card_animations(card_game_state_t *card_game, float dt);
static void trigger_on_play_effects(card_game_state_t *card_game);
static void trigger_target_effects(card_state_t *source, card_state_t *target);
static void damage_card(card_state_t *source, card_state_t *target, int damage);
static void resolve_damage(card_game_state_t *card_game, game_state_t *game_state);
static void phase_init(card_game_state_t *card_game, game_state_t *game_state);
static void phase_player_select_card_to_play(card_game_state_t *card_game, game_state_t *game_state);
static void phase_opponent_select_card_to_play(card_game_state_t *card_game, game_state_t *game_state);
static void phase_animate_playing_cards(card_game_state_t *card_game, game_state_t *game_state);
static void phase_trigger_on_play_effects(card_game_state_t *card_game, game_state_t *game_state);
static void phase_player_select_target(card_game_state_t *card_game, game_state_t *game_state);
static void phase_opponent_select_target(card_game_state_t *card_game, game_state_t *game_state);
static void phase_animate_target_effects(card_game_state_t *card_game, game_state_t *game_state);
static void phase_battle(card_game_state_t *card_game, game_state_t *game_state);
static float ai_evaluate(card_game_state_t *card_game);
static ai_selection_t ai_decision(card_game_state_t *card_game, bool is_player);

void card_game_init(card_game_state_t *card_game, game_state_t *game_state) {
    gs_dyn_array_free(card_game->player_hand);
    gs_dyn_array_free(card_game->opponent_hand);
    gs_dyn_array_free(card_game->player_hand_cache);
    gs_dyn_array_free(card_game->opponent_hand_cache);

    int render_index = 0;
    for (int i = 0; i < 6; i++) {
        card_state_t player_card = card_get_random(render_index++);
        gs_dyn_array_push(card_game->player_hand, player_card);
        gs_dyn_array_push(card_game->player_hand_cache, player_card);
        card_state_t opponent_card = card_get_random(render_index++);
        gs_dyn_array_push(card_game->opponent_hand, opponent_card);
        gs_dyn_array_push(card_game->opponent_hand_cache, opponent_card);
    }

    card_game->player_card_in_play = (card_state_t){0};
    card_game->opponent_card_in_play = (card_state_t){0};

    card_game->phase = INIT;
    update_card_visuals(card_game, game_state);
}

void card_game_update(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->phase == INIT) phase_init(card_game, game_state);
    else if (card_game->phase == PLAYER_SELECT_CARD_TO_PLAY) phase_player_select_card_to_play(card_game, game_state);
    else if (card_game->phase == OPPONENT_SELECT_CARD_TO_PLAY) phase_opponent_select_card_to_play(card_game, game_state);
    else if (card_game->phase == ANIMATE_PLAYING_CARDS) phase_animate_playing_cards(card_game, game_state);
    else if (card_game->phase == TRIGGER_ON_PLAY_EFFECTS) phase_trigger_on_play_effects(card_game, game_state);
    else if (card_game->phase == PLAYER_SELECT_TARGET) phase_player_select_target(card_game, game_state);
    else if (card_game->phase == OPPONENT_SELECT_TARGET) phase_opponent_select_target(card_game, game_state);
    else if (card_game->phase == BATTLE) phase_battle(card_game, game_state);

    float dt = gs_platform_delta_time() * card_game->game_speed;
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
    card_render_instanced(game_state->card_renderer, card_game->player_hand, gs_dyn_array_size(card_game->player_hand), &game_state->command_buffer, view_projection);
    card_render_instanced(game_state->card_renderer, card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), &game_state->command_buffer, view_projection);
    // if there is a player card in play, position and rotate correctly
    if (card_game->player_card_in_play.name != NULL) {
        card_render_instanced(game_state->card_renderer, &card_game->player_card_in_play, 1, &game_state->command_buffer, view_projection);
    }
    // if there is a opponent card in play, position and rotate correctly
    if (card_game->opponent_card_in_play.name != NULL) {
        card_render_instanced(game_state->card_renderer, &card_game->opponent_card_in_play, 1, &game_state->command_buffer, view_projection);
    }
}

static void set_phase(card_game_state_t *card_game, enum card_game_phase phase) {
    card_game->phase = phase;
    card_game->phase_timer = 0.0f;
    card_game->phase_tick = 0;
    printf("SET PHASE: %s TIME: %f\n", get_phase_name(phase), card_game->game_timer);
}

static void phase_init(card_game_state_t *card_game, game_state_t *game_state) {
    position_hand_cards(card_game, card_game->player_hand);
    position_hand_cards(card_game, card_game->opponent_hand);
    set_phase(card_game, PLAYER_SELECT_CARD_TO_PLAY);
}

static void phase_player_select_card_to_play(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->simulate_player) {
        ai_selection_t selection = ai_decision(card_game, true);
        card_game->player_selection = selection;
        card_game->player_card_in_play = card_game->player_hand[selection.hand_index];
        card_game->player_just_played_card = true;
        card_game->player_on_play_triggered = false;
        gs_dyn_array_erase(card_game->player_hand, selection.hand_index);

        // if the opponent doesn't have a card in play, transition
        // to that phase, otherwise start animating playing cards
        if (card_game->opponent_card_in_play.name == NULL) {
            set_phase(card_game, OPPONENT_SELECT_CARD_TO_PLAY);
        } else {
            set_phase(card_game, ANIMATE_PLAYING_CARDS);
        }
    } else {
        highlight_remove_all(card_game, game_state);
        highlight_playable_cards(card_game, game_state);
        if (card_game->hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) { // will always be an index in players hand here
            highlight_remove_all(card_game, game_state); // remove all highlights

            card_game->player_card_in_play = card_game->player_hand[card_game->hovered_index];
            card_game->player_just_played_card = true;
            card_game->player_on_play_triggered = false;
            gs_dyn_array_erase(card_game->player_hand, card_game->hovered_index);

            // if the opponent doesn't have a card in play, transition
            // to that phase, otherwise start animating playing cards
            if (card_game->opponent_card_in_play.name == NULL) {
                set_phase(card_game, OPPONENT_SELECT_CARD_TO_PLAY);
            } else {
                set_phase(card_game, ANIMATE_PLAYING_CARDS);
            }
        }
    }
}

static void phase_opponent_select_card_to_play(card_game_state_t *card_game, game_state_t *game_state) {
    ai_selection_t selection = ai_decision(card_game, false);
    card_game->opponent_selection = selection;
    card_game->opponent_card_in_play = card_game->opponent_hand[selection.hand_index];
    card_game->opponent_just_played_card = true;
    card_game->opponent_on_play_triggered = false;

    gs_dyn_array_erase(card_game->opponent_hand, selection.hand_index);

    set_phase(card_game, ANIMATE_PLAYING_CARDS);
}

static void phase_animate_playing_cards(card_game_state_t *card_game, game_state_t *game_state) {
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
}

static void phase_trigger_on_play_effects(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->player_just_played_card || card_game->opponent_just_played_card) {
        if (card_game->phase_tick == 1) {
            trigger_on_play_effects(card_game);
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
}

static void phase_player_select_target(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->player_just_played_card) {
        if (card_has_target_ability(&card_game->player_card_in_play.abilities)) {
            card_game->player_selecting_target_type = ANY;
        } else if (card_has_target_ability_self(&card_game->player_card_in_play.abilities)) {
            card_game->player_selecting_target_type = SELF;
        } else if (card_has_target_ability_other(&card_game->player_card_in_play.abilities)) {
            card_game->player_selecting_target_type = OTHER;
        }
        card_game->player_just_played_card = false;
    }

    if (card_game->player_selecting_target_type != NONE) { // wait while player is selecting target
        if (card_game->simulate_player) {
            if (card_game->player_selection.target_index < 10) {
                card_game->target = &card_game->player_hand[card_game->player_selection.target_index];
            } else if (card_game->player_selection.target_index < 20) {
                card_game->target = &card_game->opponent_hand[card_game->player_selection.target_index - 10];
            } else if (card_game->player_selection.target_index == 20) {
                card_game->target = &card_game->player_card_in_play;
            } else {
                card_game->target = &card_game->opponent_card_in_play;
            }
            trigger_target_effects(&card_game->player_card_in_play, card_game->target);
            resolve_damage(card_game, game_state);
            position_hand_cards(card_game, card_game->player_hand);
            position_hand_cards(card_game, card_game->opponent_hand);
            card_game->visual_update = true;
            card_game->player_selecting_target_type = NONE;
            set_phase(card_game, PLAYER_SELECT_TARGET); // set to same phase to reset phase timer
        } else {
            highlight_remove_all(card_game, game_state);
            highlight_targetable_cards(card_game, game_state, card_game->player_selecting_target_type);
            if (card_game->hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
                if (card_game->hovered_index < 10) {
                    card_game->target = &card_game->player_hand[card_game->hovered_index];
                } else if (card_game->hovered_index < 20) {
                    card_game->target = &card_game->opponent_hand[card_game->hovered_index - 10];
                } else if (card_game->hovered_index == 20) {
                    card_game->target = &card_game->player_card_in_play;
                } else {
                    card_game->target = &card_game->opponent_card_in_play;
                }
                trigger_target_effects(&card_game->player_card_in_play, card_game->target);
                resolve_damage(card_game, game_state);
                position_hand_cards(card_game, card_game->player_hand);
                position_hand_cards(card_game, card_game->opponent_hand);
                card_game->visual_update = true;
                card_game->player_selecting_target_type = NONE;
                set_phase(card_game, PLAYER_SELECT_TARGET); // set to same phase to reset phase timer
            }
        }
    } else {
        highlight_remove_all(card_game, game_state);
        if (card_game->target != NULL) {
            float begin_anim_time = 0.15;
            float end_anim_time = 0.25;
            float end_phase_time = 0.3;
            if (card_game->phase_timer_prev < begin_anim_time && card_game->phase_timer >= begin_anim_time) {
                gs_vqs_t target_transform = card_game->target->target_transform;
                target_transform.scale = gs_v3(0.9f, 0.9f, 0.9f);
                set_card_animation(card_game->target, target_transform, 0.05f);
            }
            if (card_game->phase_timer_prev < end_anim_time && card_game->phase_timer >= end_anim_time) {
                gs_vqs_t target_transform = card_game->target->target_transform;
                target_transform.scale = gs_v3(1.f, 1.f, 1.f);
                set_card_animation(card_game->target, target_transform, 0.05f);
                card_game->target = NULL;
            }
            if (card_game->phase_timer > end_phase_time) {
                set_phase(card_game, OPPONENT_SELECT_TARGET);
            }
        } else {
            set_phase(card_game, OPPONENT_SELECT_TARGET);
        }
    }
}

static void phase_opponent_select_target(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->opponent_just_played_card) {
        if (card_has_target_ability(&card_game->opponent_card_in_play.abilities)) {
            card_game->opponent_selecting_target_type = ANY;
        } else if (card_has_target_ability_self(&card_game->opponent_card_in_play.abilities)) {
            card_game->opponent_selecting_target_type = SELF;
        } else if (card_has_target_ability_other(&card_game->opponent_card_in_play.abilities)) {
            card_game->opponent_selecting_target_type = OTHER;
        }

        card_game->opponent_just_played_card = false;
    }

    if (card_game->opponent_selecting_target_type != NONE) {
        if (card_game->opponent_selection.target_index < 10) {
            card_game->target = &card_game->player_hand[card_game->opponent_selection.target_index];
        } else if (card_game->opponent_selection.target_index < 20) {
            card_game->target = &card_game->opponent_hand[card_game->opponent_selection.target_index - 10];
        } else if (card_game->opponent_selection.target_index == 20) {
            card_game->target = &card_game->player_card_in_play;
        } else if (card_game->opponent_selection.target_index == 21) {
            card_game->target = &card_game->opponent_card_in_play;
        } else {
            printf("ERROR, oppoenet target index invalid %i\n", card_game->opponent_selection.target_index);
        }

        trigger_target_effects(&card_game->opponent_card_in_play, card_game->target);
        resolve_damage(card_game, game_state);
        position_hand_cards(card_game, card_game->player_hand);
        position_hand_cards(card_game, card_game->opponent_hand);
        card_game->visual_update = true;
        card_game->opponent_selecting_target_type = NONE;
        set_phase(card_game, OPPONENT_SELECT_TARGET); // set to same phase to reset phase timer
    } else {
        if (card_game->target != NULL) {
            float begin_anim_time = 0.15;
            float end_anim_time = 0.25;
            float end_phase_time = 0.3;
            if (card_game->phase_timer_prev < begin_anim_time && card_game->phase_timer >= begin_anim_time) {
                gs_vqs_t target_transform = card_game->target->target_transform;
                target_transform.scale = gs_v3(0.8f, 0.8f, 0.8f);
                set_card_animation(card_game->target, target_transform, 0.05f);
            }
            if (card_game->phase_timer_prev < end_anim_time && card_game->phase_timer >= end_anim_time) {
                gs_vqs_t target_transform = card_game->target->target_transform;
                target_transform.scale = gs_v3(1.f, 1.f, 1.f);
                set_card_animation(card_game->target, target_transform, 0.05f);
                card_game->target = NULL;
            }
            if (card_game->phase_timer > end_phase_time) {
                set_phase(card_game, BATTLE);
            }
        } else {
            set_phase(card_game, BATTLE);
        }
    }
}

static void phase_battle(card_game_state_t *card_game, game_state_t *game_state) {
    float haste_begin_attack_time = 0.3;
    float haste_end_attack_time = 0.5;
    float haste_resolve_time = 0.7;
    float begin_attack_time = 1.0;
    float end_attack_time = 1.2;
    float attack_resolve_time = 1.5;

    if (card_game->phase_timer_prev < haste_begin_attack_time && card_game->phase_timer >= haste_begin_attack_time) {
        printf("haste begin: %f\n", card_game->phase_timer);
        card_game->player_card_attacking = card_game->player_card_in_play.current_abilities.haste && !card_game->player_card_in_play.current_abilities.frozen;
        card_game->opponent_card_attacking = card_game->opponent_card_in_play.current_abilities.haste && !card_game->opponent_card_in_play.current_abilities.frozen;
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
    if (card_game->phase_timer_prev < haste_end_attack_time && card_game->phase_timer >= haste_end_attack_time) {
        printf("haste end: %f\n", card_game->phase_timer);
        if (card_game->player_card_attacking) {
            gs_vqs_t target_transform = gs_vqs_default();
            target_transform.position = gs_v3(-2.0f, 0.f, 0.f);
            set_card_animation(&card_game->player_card_in_play, target_transform, 0.1f);
            damage_card(&card_game->player_card_in_play, &card_game->opponent_card_in_play, card_game->player_card_in_play.current_attack);
        }
        if (card_game->opponent_card_attacking) {
            gs_vqs_t target_transform = gs_vqs_default();
            target_transform.position = gs_v3(2.0f, 0.f, 0.f);
            set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.1f);
            damage_card(&card_game->opponent_card_in_play, &card_game->player_card_in_play, card_game->opponent_card_in_play.current_attack);
        }
        card_game->player_card_attacking = false;
        card_game->opponent_card_attacking = false;
        card_game->visual_update = true;
    }
    if (card_game->phase_timer_prev < haste_resolve_time && card_game->phase_timer >= haste_resolve_time) {
        printf("haste resolve: %f\n", card_game->phase_timer);
        card_game->player_card_attacking = false;
        card_game->opponent_card_attacking = false;
        resolve_damage(card_game, game_state);
        position_hand_cards(card_game, card_game->player_hand);
        position_hand_cards(card_game, card_game->opponent_hand);
    }

    // normal animation timer ticks
    if (card_game->phase_timer_prev < begin_attack_time && card_game->phase_timer >= begin_attack_time) {
        printf("attack begin: %f\n", card_game->phase_timer);
        card_game->player_card_attacking = !card_game->player_card_in_play.current_abilities.haste && !card_game->player_card_in_play.current_abilities.frozen;
        card_game->opponent_card_attacking = !card_game->opponent_card_in_play.current_abilities.haste && !card_game->opponent_card_in_play.current_abilities.frozen;
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
    if (card_game->phase_timer_prev < end_attack_time && card_game->phase_timer >= end_attack_time) {
        printf("attack end: %f\n", card_game->phase_timer);
        if (card_game->player_card_attacking) {
            gs_vqs_t target_transform = gs_vqs_default();
            target_transform.position = gs_v3(-2.0f, 0.f, 0.f);
            set_card_animation(&card_game->player_card_in_play, target_transform, 0.1f);

            damage_card(&card_game->player_card_in_play, &card_game->opponent_card_in_play, card_game->player_card_in_play.current_attack);
        }
        if (card_game->opponent_card_attacking) {
            gs_vqs_t target_transform = gs_vqs_default();
            target_transform.position = gs_v3(2.0f, 0.f, 0.f);
            set_card_animation(&card_game->opponent_card_in_play, target_transform, 0.1f);

            damage_card(&card_game->opponent_card_in_play, &card_game->player_card_in_play, card_game->opponent_card_in_play.current_attack);
        }
        card_game->player_card_attacking = false;
        card_game->opponent_card_attacking = false;
        card_game->player_card_attacking = false;
        card_game->opponent_card_attacking = false;
        // remove frozen state after attack round
        card_game->player_card_in_play.current_abilities.frozen = false;
        card_game->opponent_card_in_play.current_abilities.frozen = false;
        card_game->visual_update = true;
    }
    // normal battle tick
    if (card_game->phase_timer_prev < attack_resolve_time && card_game->phase_timer >= attack_resolve_time) {
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
        card_update_visuals(game_state->card_renderer, &card_game->player_hand[i], &game_state->immediate_draw);
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); ++i) {
        card_update_visuals(game_state->card_renderer, &card_game->opponent_hand[i], &game_state->immediate_draw);
    }
    if (card_game->player_card_in_play.name != NULL) {
        card_update_visuals(game_state->card_renderer, &card_game->player_card_in_play, &game_state->immediate_draw);
    }
    if (card_game->opponent_card_in_play.name != NULL) {
        card_update_visuals(game_state->card_renderer, &card_game->opponent_card_in_play, &game_state->immediate_draw);
    }
}

static void print_stats(card_game_state_t *card_game) {
    printf("player wins: %i, opponent wins: %i, draws: %i\n", card_game->player_wins, card_game->opponent_wins, card_game->draws);
    for (int i = 0; i < 100; i++) {
        int count = card_game->winning_card_counts[i];
        if (count > 0) {
            printf("stats %s: %i\n", card_database[i].name, count);
        }
    }
}

void resolve_damage(card_game_state_t *card_game, game_state_t *game_state) {
    bool was_a_card_destroyed = false;
    if (card_game->player_card_in_play.current_health <= 0) {
        if (card_game->player_card_in_play.abilities.regenerate) {
            card_reset(&card_game->player_card_in_play);
            card_game->player_card_in_play.abilities.regenerate = false;
            gs_dyn_array_push(card_game->player_hand, card_game->player_card_in_play);
        }
        card_game->player_card_in_play = (card_state_t){0};
        was_a_card_destroyed = true;
    }
    if (card_game->opponent_card_in_play.current_health <= 0) {
        if (card_game->opponent_card_in_play.abilities.regenerate > 0) {
            card_reset(&card_game->opponent_card_in_play);
            card_game->opponent_card_in_play.abilities.regenerate = false;
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
            if (card_game->simulate_player && card_game->simulation_count > 0) {
                card_game->simulation_count--;
                card_game->draws++;
                card_game_init(card_game, game_state);
            } else {
                print_stats(card_game);
                game_state->mode = MENU;
            }
        } else if (gs_dyn_array_size(card_game->opponent_hand) == 0
            && card_game->opponent_card_in_play.name == NULL) {
            printf("PLAYER WINS\n");
            if (card_game->simulate_player && card_game->simulation_count > 0) {
                card_game->simulation_count--;
                card_game->player_wins++;
                for (int i = 0; i < gs_dyn_array_size(card_game->player_hand_cache); i++) {
                    card_game->winning_card_counts[card_game->player_hand_cache[i].database_index]++;
                }
                card_game_init(card_game, game_state);
            } else {
                print_stats(card_game);
                game_state->mode = MENU;
            }
        } else if (gs_dyn_array_size(card_game->player_hand) == 0
            && card_game->player_card_in_play.name == NULL) {
            printf("OPPONENT WINS\n");
            if (card_game->simulate_player && card_game->simulation_count > 0) {
                card_game->simulation_count--;
                card_game->opponent_wins++;
                for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand_cache); i++) {
                    card_game->winning_card_counts[card_game->opponent_hand_cache[i].database_index]++;
                }
                card_game_init(card_game, game_state);
            } else {
                print_stats(card_game);
                game_state->mode = MENU;
            }
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
    // ward prevents a targeted effect, then is removed
    if (target->current_abilities.ward) {
        target->current_abilities.ward = false;
        return;
    }

    if (source->current_abilities.strike > 0) {
        damage_card(source, target, source->current_abilities.strike);
    }
    if (source->current_abilities.sacrifice) {
        target->current_health = 0;
    }
    target->current_attack = fmax(1, target->current_attack - source->current_abilities.dull);
    target->current_health += source->current_abilities.heal;
    target->current_attack += source->current_abilities.sharpen;
    target->current_abilities.shield = target->current_abilities.shield || source->current_abilities.bestow_shield;
    target->current_abilities.regenerate = target->current_abilities.regenerate || source->current_abilities.bestow_regenerate;
    target->current_abilities.haste = target->current_abilities.haste || source->current_abilities.bestow_haste;
    target->current_abilities.timebound = target->current_abilities.timebound || source->current_abilities.bestow_timebound;
    target->current_abilities.sacrifice = target->current_abilities.sacrifice || source->current_abilities.bestow_sacrifice;
    target->current_abilities.frozen = target->current_abilities.frozen || source->current_abilities.bestow_frozen;
    target->current_abilities.ward = target->current_abilities.ward || source->current_abilities.bestow_ward;

    if (source->current_abilities.cancel) {
        target->current_abilities = (card_abilities_t){0};
    }
}

void trigger_on_play_effects(card_game_state_t *card_game) {
    if (card_game->player_just_played_card && !card_game->player_on_play_triggered) {
        card_game->player_on_play_triggered = true;
        card_game->player_card_in_play.current_health += card_game->player_card_in_play.current_abilities.mass_heal;
        card_game->player_card_in_play.current_attack += card_game->player_card_in_play.current_abilities.mass_sharpen;
        card_game->opponent_card_in_play.current_attack = fmax(1, card_game->opponent_card_in_play.current_attack - card_game->player_card_in_play.current_abilities.mass_dull);
        damage_card(&card_game->player_card_in_play, &card_game->opponent_card_in_play, card_game->player_card_in_play.current_abilities.mass_strike);

        for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
            card_game->player_hand[i].current_health += card_game->player_card_in_play.current_abilities.mass_heal;
            card_game->player_hand[i].current_attack += card_game->player_card_in_play.current_abilities.mass_sharpen;
            card_game->player_hand[i].current_health += card_game->player_hand[i].current_abilities.charge_heal;
            card_game->player_hand[i].current_attack += card_game->player_hand[i].current_abilities.charge_sharpen;
        }
        for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
            card_game->opponent_hand[i].current_attack = fmax(1, card_game->opponent_hand[i].current_attack - card_game->player_card_in_play.current_abilities.mass_dull);
            damage_card(&card_game->player_card_in_play, &card_game->opponent_hand[i], card_game->player_card_in_play.current_abilities.mass_strike);
        }
    }
    if (card_game->opponent_just_played_card && !card_game->opponent_on_play_triggered) {
        card_game->opponent_on_play_triggered = true;
        card_game->opponent_card_in_play.current_health += card_game->opponent_card_in_play.current_abilities.mass_heal;
        card_game->opponent_card_in_play.current_attack += card_game->opponent_card_in_play.current_abilities.mass_sharpen;
        card_game->player_card_in_play.current_attack = fmax(1, card_game->player_card_in_play.current_attack - card_game->opponent_card_in_play.current_abilities.mass_dull);
        damage_card(&card_game->opponent_card_in_play, &card_game->player_card_in_play, card_game->opponent_card_in_play.current_abilities.mass_strike);
        for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
            card_game->opponent_hand[i].current_health += card_game->opponent_card_in_play.current_abilities.mass_heal;
            card_game->opponent_hand[i].current_attack += card_game->opponent_card_in_play.current_abilities.mass_sharpen;
            card_game->opponent_hand[i].current_health += card_game->opponent_hand[i].current_abilities.charge_heal;
            card_game->opponent_hand[i].current_attack += card_game->opponent_hand[i].current_abilities.charge_sharpen;
        }
        for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
            card_game->player_hand[i].current_attack = fmax(1, card_game->player_hand[i].current_attack - card_game->opponent_card_in_play.current_abilities.mass_dull);
            damage_card(&card_game->opponent_card_in_play, &card_game->player_hand[i], card_game->opponent_card_in_play.current_abilities.mass_strike);
        }
    }
}

static void damage_card(card_state_t *source, card_state_t *target, int damage) {
    if (damage <= 0) return;

    if (target->current_abilities.shield) {
        target->current_abilities.shield = false;
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
        if (card_game->player_hand[i].current_abilities.timebound) has_timebound = true;
    }

    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (!has_timebound || card_game->player_hand[i].current_abilities.timebound) {
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

static void highlight_targetable_cards(card_game_state_t *card_game,  game_state_t *game_state, enum card_game_target_type type) {
    if (type == ANY || type == SELF) {
        for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
            if (!card_game->player_hand[i].selectable) {
                card_game->player_hand[i].selectable = true;
                card_game->visual_update = true;
            }
        }
    }
    if (type == ANY || type == OTHER) {
        for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
            if (!card_game->opponent_hand[i].selectable) {
                card_game->opponent_hand[i].selectable = true;
                card_game->visual_update = true;
            }
        }
    }
    if (type == ANY || type == SELF) {
        if (card_game->player_card_in_play.name != NULL && !card_game->player_card_in_play.selectable) {
            card_game->player_card_in_play.selectable = true;
            card_game->visual_update = true;
        }
    }
    if (type == ANY || type == OTHER) {
        if (card_game->opponent_card_in_play.name != NULL && !card_game->opponent_card_in_play.selectable) {
            card_game->opponent_card_in_play.selectable = true;
            card_game->visual_update = true;
        }
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

static float ai_evaluate_card(card_state_t *card, float other_attack, float other_health) {
    float attack_weight = 1.f;
    float health_weight = 1.f;
    float shield_weight = 1.f;
    float haste_weight = 1.f;
    float renegerate_weight = 1.f;
    float timebound_weight = 0.f;
    float sacrifice_weight = 0.f;
    float frozen_weight = -0.2f;
    float ward_weight = 0.5f;
    float cancel_weight = 0.f;

    float score = 0;
    float base_score = card->current_attack * attack_weight + card->current_health * health_weight;

    score = base_score;
    score += card->current_abilities.shield * shield_weight * base_score;
    score += card->current_abilities.haste * haste_weight * base_score;
    score += card->current_abilities.regenerate * renegerate_weight * score;
    score += card->current_abilities.timebound * timebound_weight * score;
    score += card->current_abilities.sacrifice * sacrifice_weight * score;
    score += card->current_abilities.frozen * frozen_weight * score;
    score += card->current_abilities.ward * ward_weight * score;
    score += card->current_abilities.cancel * cancel_weight * score;

    if (card->current_attack >= other_health) score += other_attack + other_health;
    if (card->current_health <= other_attack) score -= card->current_attack + card->current_health;

    return score;
}

// higher numbers means more favorable for the player, lower is more favorable for opponent
static float ai_evaluate(card_game_state_t *card_game) {
    // get the basic cases out of the way, ending the game is +-1000
    if (card_game->player_card_in_play.name == NULL && gs_dyn_array_size(card_game->player_hand) == 0) {
        return -1000;
    }
    if (card_game->opponent_card_in_play.name == NULL && gs_dyn_array_size(card_game->opponent_hand) == 0) {
        return 1000;
    }
    float attack_weight = 1.f;
    float health_weight = 1.f;
    float score = 0;
    // if the opponent has a card in play, pass that into evaluate card

    bool has_opposing_card = card_game->opponent_card_in_play.name != NULL;
    float other_avg_attack = 0;
    float other_avg_health = 0;
    int hand_size = gs_dyn_array_size(card_game->player_hand);
    for (int i = 0; i < hand_size; i++) {
        score += ai_evaluate_card(&card_game->player_hand[i], 0, 0);
        if (!has_opposing_card) {
            other_avg_attack += card_game->player_hand[i].current_attack;
            other_avg_health += card_game->player_hand[i].current_health;
        }
    }
    if (has_opposing_card) {
        score += ai_evaluate_card(&card_game->player_card_in_play, card_game->opponent_card_in_play.current_attack, card_game->opponent_card_in_play.current_health);
    } else {
        score += ai_evaluate_card(&card_game->player_card_in_play, hand_size == 0 ? 0 : other_avg_attack/ hand_size, hand_size == 0 ? 0 : other_avg_health/ hand_size);
    }

    has_opposing_card = card_game->player_card_in_play.name != NULL;
    other_avg_attack = 0;
    other_avg_health = 0;
    hand_size = gs_dyn_array_size(card_game->opponent_hand);
    for (int i = 0; i < hand_size; i++) {
        score -= ai_evaluate_card(&card_game->opponent_hand[i], 0, 0);
        if (!has_opposing_card) {
            other_avg_attack += card_game->opponent_hand[i].current_attack;
            other_avg_health += card_game->opponent_hand[i].current_health;
        }
    }
    if (has_opposing_card) {
        score -= ai_evaluate_card(&card_game->opponent_card_in_play, card_game->player_card_in_play.current_attack, card_game->player_card_in_play.current_health);
    } else {
        score -= ai_evaluate_card(&card_game->opponent_card_in_play, hand_size == 0 ? 0 : other_avg_attack/ hand_size, hand_size == 0 ? 0 : other_avg_health/ hand_size);
    }

    return score;
}

static card_game_state_t copy_state(card_game_state_t *card_game) {
    card_game_state_t tmp = *card_game;
    tmp.player_hand = NULL;
    tmp.opponent_hand = NULL;
    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
        gs_dyn_array_push(tmp.player_hand, card_game->player_hand[i]);
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
        gs_dyn_array_push(tmp.opponent_hand, card_game->opponent_hand[i]);
    }

    return tmp;
}

static ai_selection_t ai_decision(card_game_state_t *card_game, bool is_player) {
    // printf("--- decision start. is_player: %i\n", is_player);
    int hand_index = 0;
    int target_index = -1;
    float score = is_player ? -1000 : 1000;
    float eval = 0;

    card_game_state_t tmp_cg = copy_state(card_game);
    gs_dyn_array(card_state_t) hand = is_player ? tmp_cg.player_hand : tmp_cg.opponent_hand;

    gs_dyn_array(int) timebound_indices = NULL;
    for (int i = 0; i < gs_dyn_array_size(hand); i++) {
        if (hand[i].current_abilities.timebound) gs_dyn_array_push(timebound_indices, i);
    }

    gs_dyn_array(int) itteration_indices = NULL;
    if (gs_dyn_array_size(timebound_indices) > 0) {
        itteration_indices = timebound_indices;
    } else {
        for (int i = 0; i < gs_dyn_array_size(hand); i++) {
            gs_dyn_array_push(itteration_indices, i);
        }
    }

    for (int i = 0; i < gs_dyn_array_size(itteration_indices); i++) {
        if (is_player) {
            tmp_cg.player_card_in_play = hand[itteration_indices[i]];
            tmp_cg.player_just_played_card = true;
            tmp_cg.player_on_play_triggered = false;
        } else {
            tmp_cg.opponent_card_in_play = hand[itteration_indices[i]];
            tmp_cg.opponent_just_played_card = true;
            tmp_cg.opponent_on_play_triggered = false;
        }

        gs_dyn_array_erase(hand, itteration_indices[i]);

        trigger_on_play_effects(&tmp_cg);
        if (is_player) {
            tmp_cg.player_just_played_card = false;
        } else {
            tmp_cg.opponent_just_played_card = false;
        }

        card_game_state_t tmp_cg_target = copy_state(&tmp_cg);
        enum card_game_target_type target_type = NONE;
        if (is_player) {
            if (card_has_target_ability(&tmp_cg_target.player_card_in_play.current_abilities)) target_type = ANY;
            else if (card_has_target_ability_self(&tmp_cg_target.player_card_in_play.current_abilities)) target_type = SELF;
            else if (card_has_target_ability_other(&tmp_cg_target.player_card_in_play.current_abilities)) target_type = OTHER;
        } else {
            if (card_has_target_ability(&tmp_cg_target.opponent_card_in_play.current_abilities)) target_type = ANY;
            else if (card_has_target_ability_self(&tmp_cg_target.opponent_card_in_play.current_abilities)) target_type = SELF;
            else if (card_has_target_ability_other(&tmp_cg_target.opponent_card_in_play.current_abilities)) target_type = OTHER;
        }

        if (target_type == NONE) {
            eval = ai_evaluate(&tmp_cg_target);

            // if (is_player) printf("eval: %f playing card at index %i, name: %s | NO TARGETS\n", eval, itteration_indices[i], tmp_cg_target.player_card_in_play.name);
            // else printf("eval: %f playing card at index %i, name: %s | NO TARGETS\n", eval, itteration_indices[i], tmp_cg_target.opponent_card_in_play.name);
            if (is_player && eval > score) {
                score = eval;
                hand_index = itteration_indices[i];
                target_index = -1;
            }
            if (!is_player && eval < score) {
                score = eval;
                hand_index = itteration_indices[i];
                target_index = -1;
            }

            tmp_cg_target = copy_state(&tmp_cg);
        } else {
            if (target_type == ANY || target_type == OTHER) {

                gs_dyn_array(card_state_t) other_hand = is_player ? tmp_cg_target.opponent_hand : tmp_cg_target.player_hand;

                for (int j = 0; j < gs_dyn_array_size(other_hand); j++) {
                    if (is_player) trigger_target_effects(&tmp_cg_target.player_card_in_play, &other_hand[j]);
                    else trigger_target_effects(&tmp_cg_target.opponent_card_in_play, &other_hand[j]);

                    eval = ai_evaluate(&tmp_cg_target);

                    // if (is_player) printf("eval: %f playing card at index %i, name: %s | OTHER\n", eval, itteration_indices[i], tmp_cg_target.player_card_in_play.name);
                    // else printf("eval: %f playing card at index %i, name: %s | OTHER\n", eval, itteration_indices[i], tmp_cg_target.opponent_card_in_play.name);
                    if (is_player && eval > score) {
                        score = eval;
                        hand_index = itteration_indices[i];
                        target_index = j + 10;
                    }
                    if (!is_player && eval < score) {
                        score = eval;
                        hand_index = itteration_indices[i];
                        target_index = j;
                    }

                    tmp_cg_target = copy_state(&tmp_cg);
                    other_hand = is_player ? tmp_cg_target.opponent_hand : tmp_cg_target.player_hand;
                }

                if (is_player) trigger_target_effects(&tmp_cg_target.player_card_in_play, &tmp_cg_target.opponent_card_in_play);
                else trigger_target_effects(&tmp_cg_target.opponent_card_in_play, &tmp_cg_target.player_card_in_play);

                eval = ai_evaluate(&tmp_cg_target);

                // if (is_player) printf("eval: %f playing card at index %i, name: %s | OTHER\n", eval, itteration_indices[i], tmp_cg_target.player_card_in_play.name);
                // else printf("eval: %f playing card at index %i, name: %s | OTHER\n", eval, itteration_indices[i], tmp_cg_target.opponent_card_in_play.name);
                if (is_player && eval > score) {
                    score = eval;
                    hand_index = itteration_indices[i];
                    target_index = 21;
                }
                if (!is_player && eval < score) {
                    score = eval;
                    hand_index = itteration_indices[i];
                    target_index = 20;
                }

                tmp_cg_target = copy_state(&tmp_cg);
            }
            if (target_type == ANY || target_type == SELF) {

                gs_dyn_array(card_state_t) self_hand = is_player ? tmp_cg_target.player_hand : tmp_cg_target.opponent_hand;

                for (int j = 0; j < gs_dyn_array_size(self_hand); j++) {
                    if (is_player) trigger_target_effects(&tmp_cg_target.player_card_in_play, &self_hand[j]);
                    else trigger_target_effects(&tmp_cg_target.opponent_card_in_play, &self_hand[j]);

                    eval = ai_evaluate(&tmp_cg_target);

                    // if (is_player) printf("eval: %f playing card at index %i, name: %s | SELF\n", eval, itteration_indices[i], tmp_cg_target.player_card_in_play.name);
                    // else printf("eval: %f playing card at index %i, name: %s | SELF\n", eval, itteration_indices[i], tmp_cg_target.opponent_card_in_play.name);
                    if (is_player && eval > score) {
                        score = eval;
                        hand_index = itteration_indices[i];
                        target_index = j;
                    }
                    if (!is_player && eval < score) {
                        score = eval;
                        hand_index = itteration_indices[i];
                        target_index = j + 10;
                    }

                    tmp_cg_target = copy_state(&tmp_cg);
                    self_hand = is_player ? tmp_cg_target.player_hand : tmp_cg_target.opponent_hand;
                }

                if (is_player) trigger_target_effects(&tmp_cg_target.player_card_in_play, &tmp_cg_target.player_card_in_play);
                else trigger_target_effects(&tmp_cg_target.opponent_card_in_play, &tmp_cg_target.opponent_card_in_play);

                eval = ai_evaluate(&tmp_cg_target);

                // if (is_player) printf("eval: %f playing card at index %i, name: %s | SELF\n", eval, itteration_indices[i], tmp_cg_target.player_card_in_play.name);
                // else printf("eval: %f playing card at index %i, name: %s | SELF\n", eval, itteration_indices[i], tmp_cg_target.opponent_card_in_play.name);
                if (is_player && eval > score) {
                    score = eval;
                    hand_index = itteration_indices[i];
                    target_index = 20;
                }
                if (!is_player && eval < score) {
                    score = eval;
                    hand_index = itteration_indices[i];
                    target_index = 21;
                }

                tmp_cg_target = copy_state(&tmp_cg);
            }
        }

        tmp_cg = copy_state(card_game);
        hand = is_player ? tmp_cg.player_hand : tmp_cg.opponent_hand;
    }
    // printf("--- opp decision end (hand: %i target: %i)\n", hand_index, target_index);


    return (ai_selection_t){.hand_index = hand_index, .target_index = target_index};
}

#endif
