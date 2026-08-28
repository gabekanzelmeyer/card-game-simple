#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "util_card.h"
#include "util_game.h"

// macro that allows for erasing and element from a gs_dyn_array and keeping the order
// used to remove cards from player / opponent hands
#define gs_dyn_array_erase(__ARR, __IDX)\
do {\
    if ((__ARR) && (uint32_t)(__IDX) < gs_dyn_array_size(__ARR)) {\
        memmove(\
        &(__ARR)[__IDX],\
        &(__ARR)[(__IDX) + 1],\
        (gs_dyn_array_size(__ARR) - (__IDX) - 1) * sizeof(*(__ARR))\
        );\
        gs_dyn_array_head(__ARR)->size--;\
    }\
} while (0)

#define HOVER_SCREEN_SIZE 300
#define HAND_SPACING 1.5f
#define HAND_FAN_ANGLE 2.0f
#define HAND_CURVE_AMOUNT 0.02f
#define HAND_POSITION_OFFSET 2.6f

enum card_game_phase {
    INIT,
    PLAYER_SELECT_CARD_TO_PLAY,
    OPPONENT_SELECT_CARD_TO_PLAY,
    ANIMATE_PLAYING_CARDS,
    DO_ON_PLAY_EFFECTS,
    PLAYER_SELECT_TARGET,
    OPPONENT_SELECT_TARGET,
    ANIMATE_TARGET_EFFECT,
    BATTLE
};

typedef struct {
    enum card_game_phase phase;
    gs_dyn_array(card_state_t) player_hand;
    gs_dyn_array(card_state_t) opponent_hand;
    card_state_t player_in_play_card;
    card_state_t opponent_in_play_card;
    float battle_timer;
    bool player_card_played;
    bool opponent_card_played;
    bool player_selecting_target;
    bool opponent_selecting_target;
    bool player_card_attacking;
    bool opponent_card_attacking;
    int hovered_index; // -1 means nothing, 0-5 = player hand, 10-15 = opponent hand, 20 = player in play card, 21 = oppoent in play card
    int selected_index;
    bool visual_update;
} card_game_state_t;

static void card_game_set_animation(card_state_t *card, gs_vqs_t target_transform, float duration);

void card_game_position_hand(card_state_t *cards, int count, float spacing, float tilt, float curve_amount, float y_offset) {
    float start_x = -spacing * (float)count / 2.f + spacing / 2.f;
    float start_tilt = tilt * (float)count / 2.f - tilt / 2.f;
    for (int i = 0; i < count; i++) {
        gs_vqs_t target = gs_vqs_default();
        target.position.x = start_x + i * spacing;
        target.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * curve_amount * (1.f / spacing) + y_offset;
        target.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * tilt)), gs_v3(0, 0, 1));
        card_game_set_animation(&cards[i], target, 0.3f);
    }
}

void card_game_update_all_cards(card_game_state_t *card_game, gs_immediate_draw_t *immediate_draw) {
    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); ++i) {
        card_update_visuals(&card_game->player_hand[i], immediate_draw);
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); ++i) {
        card_update_visuals(&card_game->opponent_hand[i], immediate_draw);
    }
    if (card_game->player_in_play_card.name != NULL) {
        card_update_visuals(&card_game->player_in_play_card, immediate_draw);
    }
    if (card_game->opponent_in_play_card.name != NULL) {
        card_update_visuals(&card_game->opponent_in_play_card, immediate_draw);
    }
}

void card_game_init(card_game_state_t *card_game, gs_immediate_draw_t *immediate_draw) {
    gs_dyn_array_free(card_game->player_hand);
    gs_dyn_array_free(card_game->opponent_hand);

    int render_index = 0;
    for (int i = 0; i < 6; i++) {
        gs_dyn_array_push(card_game->player_hand, card_get_random(render_index++));
        gs_dyn_array_push(card_game->opponent_hand, card_get_random(render_index++));
    }

    card_game->player_in_play_card = (card_state_t){0};
    card_game->opponent_in_play_card = (card_state_t){0};

    card_game->phase = INIT;

    card_game_update_all_cards(card_game, immediate_draw);
}

void card_game_resolve_damage(card_game_state_t *card_game, game_state_t *game_state) {
    bool was_a_card_destroyed = false;
    if (card_game->player_in_play_card.current_health <= 0) {
        if (card_game->player_in_play_card.abilities.regenerate > 0) {
            int regen = --card_game->player_in_play_card.abilities.regenerate;
            card_reset_stats(&card_game->player_in_play_card);
            card_game->player_in_play_card.abilities.regenerate = regen;
            gs_dyn_array_push(card_game->player_hand, card_game->player_in_play_card);
        }
        card_game->player_in_play_card = (card_state_t){0};
        was_a_card_destroyed = true;
    }
    if (card_game->opponent_in_play_card.current_health <= 0) {
        if (card_game->opponent_in_play_card.abilities.regenerate > 0) {
            int regen = --card_game->opponent_in_play_card.abilities.regenerate;
            card_reset_stats(&card_game->opponent_in_play_card);
            card_game->opponent_in_play_card.abilities.regenerate = regen;
            gs_dyn_array_push(card_game->opponent_hand, card_game->opponent_in_play_card);
        }
        card_game->opponent_in_play_card = (card_state_t){0};
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
            printf("opp in card hand destroyed\n");
            gs_dyn_array_erase(card_game->opponent_hand, i);
            was_a_card_destroyed = true;
            // decrement i because if multiple cards are deleted we need the
            // index to stay on the current index to correctly remove future cards
            i--;
        }
    }

    card_game_update_all_cards(card_game, &game_state->immediate_draw);

    if (was_a_card_destroyed) {
        if (gs_dyn_array_size(card_game->opponent_hand) == 0
            && gs_dyn_array_size(card_game->player_hand) == 0
            && card_game->player_in_play_card.name == NULL
            && card_game->opponent_in_play_card.name == NULL) {
            printf("DRAW\n");
            game_state->mode = MENU;
        } else if (gs_dyn_array_size(card_game->opponent_hand) == 0
            && card_game->opponent_in_play_card.name == NULL) {
            printf("PLAYER WINS\n");
            game_state->mode = MENU;
        } else if (gs_dyn_array_size(card_game->player_hand) == 0
            && card_game->player_in_play_card.name == NULL) {
            printf("OPPONENT WINS\n");
            game_state->mode = MENU;
        } else {
            if (card_game->player_in_play_card.name == NULL) {
                card_game->phase = PLAYER_SELECT_CARD_TO_PLAY;
            } else if (card_game->opponent_in_play_card.name == NULL) {
                card_game->phase = OPPONENT_SELECT_CARD_TO_PLAY;
            }
        }
    }
}

void card_game_do_card_played(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->player_card_played) {
        // if (card_game->player_in_play_card.abilities.strike > 0) {
        //     if (card_game->opponent_in_play_card.current_abilities.shield > 0) {
        //         card_game->opponent_in_play_card.current_abilities.shield--;
        //     } else {
        //         card_game->opponent_in_play_card.current_health -= card_game->player_in_play_card.abilities.strike;
        //     }
        // }
        if (card_game->player_in_play_card.abilities.disarm > 0) {
            card_game->opponent_in_play_card.current_attack = fmax(1, card_game->opponent_in_play_card.current_attack - card_game->player_in_play_card.abilities.disarm);
        }
        if (card_game->player_in_play_card.abilities.warcry > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_attack += card_game->player_in_play_card.abilities.warcry;
            }
        }
        if (card_game->player_in_play_card.abilities.wellspring > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_health += card_game->player_in_play_card.abilities.wellspring;
            }
        }
        if (card_game->player_in_play_card.abilities.swipe > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->opponent_hand[i].current_health -= card_game->player_in_play_card.abilities.swipe;
            }
        }
        if (card_game->player_in_play_card.abilities.cripple > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                card_game->opponent_hand[i].current_attack = fmax(1, card_game->opponent_hand[i].current_attack - card_game->player_in_play_card.abilities.cripple);
            }
        }
        if (card_game->player_in_play_card.abilities.channel > 0) {
            // increase this cards stats
            card_game->player_in_play_card.current_attack += card_game->player_in_play_card.abilities.channel;
            card_game->player_in_play_card.current_health += card_game->player_in_play_card.abilities.channel;
            // increase opponent card in play if colors match
            if (card_do_colors_match(card_game->player_in_play_card, card_game->opponent_in_play_card)) {
                card_game->opponent_in_play_card.current_attack += card_game->player_in_play_card.abilities.channel;
                card_game->opponent_in_play_card.current_health += card_game->player_in_play_card.abilities.channel;
            }
            // increase all hand cards if colors match
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                if (card_do_colors_match(card_game->player_in_play_card, card_game->player_hand[i])) {
                    card_game->player_hand[i].current_attack += card_game->player_in_play_card.abilities.channel;
                    card_game->player_hand[i].current_health += card_game->player_in_play_card.abilities.channel;
                }
            }
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                if (card_do_colors_match(card_game->player_in_play_card, card_game->opponent_hand[i])) {
                    card_game->opponent_hand[i].current_attack += card_game->player_in_play_card.abilities.channel;
                    card_game->opponent_hand[i].current_health += card_game->player_in_play_card.abilities.channel;
                }
            }
        }
        if (card_game->player_in_play_card.abilities.sacrifice > 0 && gs_dyn_array_size(card_game->player_hand)> 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->player_hand));
            gs_dyn_array_erase(card_game->player_hand, random_index);
        }
        if (card_game->player_in_play_card.abilities.assassinate > 0 && gs_dyn_array_size(card_game->opponent_hand)> 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->opponent_hand));
            gs_dyn_array_erase(card_game->opponent_hand, random_index);
        }
    }
    if (card_game->opponent_card_played) {
        // if (card_game->opponent_in_play_card.abilities.strike > 0) {
        //     if (card_game->player_in_play_card.current_abilities.shield > 0) {
        //         card_game->player_in_play_card.current_abilities.shield--;
        //     } else {
        //         card_game->player_in_play_card.current_health -= card_game->opponent_in_play_card.abilities.strike;
        //     }
        // }
        if (card_game->opponent_in_play_card.abilities.disarm > 0) {
            card_game->player_in_play_card.current_attack = fmax(1, card_game->player_in_play_card.current_attack - card_game->opponent_in_play_card.abilities.disarm);
        }
        if (card_game->opponent_in_play_card.abilities.warcry > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                card_game->opponent_hand[i].current_attack += card_game->opponent_in_play_card.abilities.warcry;
            }
        }
        if (card_game->opponent_in_play_card.abilities.wellspring > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                card_game->opponent_hand[i].current_health += card_game->opponent_in_play_card.abilities.wellspring;
            }
        }
        if (card_game->opponent_in_play_card.abilities.swipe > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_health -= card_game->opponent_in_play_card.abilities.swipe;
            }
        }
        if (card_game->opponent_in_play_card.abilities.cripple > 0) {
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                card_game->player_hand[i].current_attack = fmax(1, card_game->player_hand[i].current_attack - card_game->opponent_in_play_card.abilities.cripple);
            }
        }
        if (card_game->opponent_in_play_card.abilities.channel > 0) {
            // increase this cards stats
            card_game->opponent_in_play_card.current_attack += card_game->opponent_in_play_card.abilities.channel;
            card_game->opponent_in_play_card.current_health += card_game->opponent_in_play_card.abilities.channel;
            // increase opponent card in play if colors match
            if (card_do_colors_match(card_game->opponent_in_play_card, card_game->player_in_play_card)) {
                card_game->player_in_play_card.current_attack += card_game->opponent_in_play_card.abilities.channel;
                card_game->player_in_play_card.current_health += card_game->opponent_in_play_card.abilities.channel;
            }
            // increase all hand card stats if colors match
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                if (card_do_colors_match(card_game->opponent_in_play_card, card_game->player_hand[i])) {
                    card_game->player_hand[i].current_attack += card_game->opponent_in_play_card.abilities.channel;
                    card_game->player_hand[i].current_health += card_game->opponent_in_play_card.abilities.channel;
                }
            }
            for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
                if (card_do_colors_match(card_game->opponent_in_play_card, card_game->opponent_hand[i])) {
                    card_game->opponent_hand[i].current_attack += card_game->opponent_in_play_card.abilities.channel;
                    card_game->opponent_hand[i].current_health += card_game->opponent_in_play_card.abilities.channel;
                }
            }
        }
        if (card_game->opponent_in_play_card.abilities.sacrifice > 0 && gs_dyn_array_size(card_game->opponent_hand) > 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->opponent_hand));
            gs_dyn_array_erase(card_game->opponent_hand, random_index);
        }
        if (card_game->opponent_in_play_card.abilities.assassinate > 0 && gs_dyn_array_size(card_game->player_hand) > 0) {
            int random_index = (rand() % gs_dyn_array_size(card_game->player_hand));
            gs_dyn_array_erase(card_game->player_hand, random_index);
        }
    }

    card_game_resolve_damage(card_game, game_state);
    card_game_update_all_cards(card_game, &game_state->immediate_draw);
}

void card_game_do_player_card_attack_opponent(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->opponent_in_play_card.current_abilities.shield > 0) {
        card_game->opponent_in_play_card.current_abilities.shield--;
    } else {
        card_game->opponent_in_play_card.current_health -= card_game->player_in_play_card.current_attack;
    }
    card_game_update_all_cards(card_game, &game_state->immediate_draw);
}

void card_game_do_opponent_card_attack_player(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->player_in_play_card.current_abilities.shield > 0) {
        card_game->player_in_play_card.current_abilities.shield--;
    } else {
        card_game->player_in_play_card.current_health -= card_game->opponent_in_play_card.current_attack;
    }
    card_game_update_all_cards(card_game, &game_state->immediate_draw);
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

static void card_game_set_animation(card_state_t *card, gs_vqs_t target_transform, float duration) {
    card->prev_transform.position = card->transform.position;
    card->prev_transform.rotation = card->transform.rotation;
    card->prev_transform.scale = card->transform.scale;
    card->target_transform.position = target_transform.position;
    card->target_transform.rotation = target_transform.rotation;
    card->target_transform.scale = target_transform.scale;
    card->anim_duration = duration;
    card->lerp = 0.0f;
}

static void card_game_animate_card_transforms(card_game_state_t *card_game, float dt) {
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
    if (card_game->player_in_play_card.name != NULL) {
        card_game->player_in_play_card.lerp += dt / card_game->player_in_play_card.anim_duration;
        if (card_game->player_in_play_card.lerp >= 1) {
            card_game->player_in_play_card.lerp = 1;
        }
        card_game_lerp_card_transform(&card_game->player_in_play_card, dt);
    }
    if (card_game->opponent_in_play_card.name != NULL) {
        card_game->opponent_in_play_card.lerp += dt / card_game->opponent_in_play_card.anim_duration;
        if (card_game->opponent_in_play_card.lerp >= 1) {
            card_game->opponent_in_play_card.lerp = 1;
        }
        card_game_lerp_card_transform(&card_game->opponent_in_play_card, dt);
    }
}

static bool card_game_battle_timer_trigger_happens(card_game_state_t *card_game, float prev_timer, float current_timer, float trigger, float resolution) {
    if (floorf((prev_timer - trigger) / resolution) * resolution
        < floorf((current_timer - trigger) / resolution) * resolution) {
        return true;
    }
    return false;
}

static void card_game_set_player_playable_cards_selectable(card_game_state_t *card_game,  game_state_t *game_state) {
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
    if (card_game->player_in_play_card.name != NULL && !card_game->player_in_play_card.selectable) {
        card_game->player_in_play_card.selectable = true;
        card_game->visual_update = true;
    }
    if (card_game->opponent_in_play_card.name != NULL && !card_game->opponent_in_play_card.selectable) {
        card_game->opponent_in_play_card.selectable = true;
        card_game->visual_update = true;
    }
}

static void card_game_set_hovered_card(card_game_state_t *card_game, game_state_t *game_state) {
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
                card_game_set_animation(&card_game->player_hand[i], target, 0.1f);
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
                card_game_set_animation(&card_game->opponent_hand[i], target, 0.1f);
            }
            hovered_index = i + 10; // opponent hand hovered indices is 10-15
            break;
        }
    }
    if (card_game->player_in_play_card.name != NULL) {
        world_to_screen(card_game->player_in_play_card.transform.position, &screen_pos, view_projection, fbw, fbh);
        if (card_game->player_in_play_card.selectable && gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE) {
            if (!card_game->player_in_play_card.hovered) {
                card_game->player_in_play_card.hovered = true;
                gs_vqs_t target = card_game->player_in_play_card.target_transform;
                target.scale = gs_v3(1.2f, 1.2f, 1.2f);
                card_game_set_animation(&card_game->player_in_play_card, target, 0.1f);
            }
            hovered_index = 20; // 20 is player in play card
        }
    }
    if (card_game->opponent_in_play_card.name != NULL) {
        world_to_screen(card_game->opponent_in_play_card.transform.position, &screen_pos, view_projection, fbw, fbh);
        if (card_game->opponent_in_play_card.selectable && gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE) {
            if (!card_game->opponent_in_play_card.hovered) {
                card_game->opponent_in_play_card.hovered = true;
                gs_vqs_t target = card_game->opponent_in_play_card.target_transform;
                target.scale = gs_v3(1.2f, 1.2f, 1.2f);
                card_game_set_animation(&card_game->opponent_in_play_card, target, 0.1f);
            }
            hovered_index = 21; // 21 is opponent in play card
        }
    }


    // change scale of cards that aren't hovered
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        if (card_game->player_hand[i].hovered && i != hovered_index) {
            card_game->player_hand[i].hovered = false;
            gs_vqs_t target = card_game->player_hand[i].target_transform;
            target.scale = gs_v3(1.0f, 1.0f, 1.0f);
            card_game_set_animation(&card_game->player_hand[i], target, 0.1f);
        }
    }
    for (int i = gs_dyn_array_size(card_game->opponent_hand) - 1; i >= 0; i--) {
        if (card_game->opponent_hand[i].hovered && (i + 10) != hovered_index) {
            card_game->opponent_hand[i].hovered = false;
            gs_vqs_t target = card_game->opponent_hand[i].target_transform;
            target.scale = gs_v3(1.0f, 1.0f, 1.0f);
            card_game_set_animation(&card_game->opponent_hand[i], target, 0.1f);
        }
    }
    if (card_game->player_in_play_card.hovered && 20 != hovered_index) {
        card_game->player_in_play_card.hovered = false;
        gs_vqs_t target = card_game->player_in_play_card.target_transform;
        target.scale = gs_v3(1.0f, 1.0f, 1.0f);
        card_game_set_animation(&card_game->player_in_play_card, target, 0.1f);
    }
    if (card_game->opponent_in_play_card.hovered && 21 != hovered_index) {
        card_game->opponent_in_play_card.hovered = false;
        gs_vqs_t target = card_game->opponent_in_play_card.target_transform;
        target.scale = gs_v3(1.0f, 1.0f, 1.0f);
        card_game_set_animation(&card_game->opponent_in_play_card, target, 0.1f);
    }

    card_game->hovered_index = hovered_index;
}


void card_game_update(card_game_state_t *card_game, game_state_t *game_state) {
    float dt = gs_platform_delta_time();
    if (card_game->phase == INIT) {
        card_game_position_hand(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), HAND_SPACING, HAND_FAN_ANGLE, -HAND_CURVE_AMOUNT, -HAND_POSITION_OFFSET);
        card_game_position_hand(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), HAND_SPACING, -HAND_FAN_ANGLE, HAND_CURVE_AMOUNT, HAND_POSITION_OFFSET);

        card_game->phase = PLAYER_SELECT_CARD_TO_PLAY;

    } if (card_game->phase == PLAYER_SELECT_CARD_TO_PLAY) {
        card_game_set_player_playable_cards_selectable(card_game, game_state);

        if (card_game->hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
            // if there are timebound hands in the players hand, one of them must be played
            if (card_game->player_hand[card_game->hovered_index].selectable) {
                card_game->player_in_play_card = card_game->player_hand[card_game->hovered_index ];
                card_game->player_card_played = true;

                gs_dyn_array_erase(card_game->player_hand, card_game->hovered_index );

                // once we set the plater "in play" card, see if there is an opponent "in play" card,
                // if so, move to the battle stage. otherwise, move to the opponent play card stage
                if (card_game->opponent_in_play_card.name == NULL) {
                    card_game->phase = OPPONENT_SELECT_CARD_TO_PLAY;
                } else {
                    card_game->phase = ANIMATE_PLAYING_CARDS;
                }
            }
        }
    } else if (card_game->phase == OPPONENT_SELECT_CARD_TO_PLAY) {
        // if there are timebound hands in the opponents hand, one of them must be played
        // gather timebound indices
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

        card_game->opponent_in_play_card = card_game->opponent_hand[random_index];
        card_game->opponent_card_played = true;

        gs_dyn_array_erase(card_game->opponent_hand, random_index);

        card_game->phase = ANIMATE_PLAYING_CARDS;

    } else if (card_game->phase == ANIMATE_PLAYING_CARDS) {
        if (card_game->player_card_played) {
            card_game_set_animation(&card_game->player_in_play_card,
                (gs_vqs_t) {
                    .position = gs_v3(-2., 0.f, 0.f),
                    .rotation = gs_quat_default(),
                    .scale = gs_v3(1.f, 1.f, 1.f)
                },
                0.1f);
            card_game_position_hand(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), HAND_SPACING, HAND_FAN_ANGLE, -HAND_CURVE_AMOUNT, -HAND_POSITION_OFFSET);
        }
         if (card_game->opponent_card_played) {
            card_game_set_animation(&card_game->opponent_in_play_card,
                (gs_vqs_t) {
                    .position = gs_v3(2., 0.f, 0.f),
                    .rotation = gs_quat_default(),
                    .scale = gs_v3(1.f, 1.f, 1.f)
                },
            0.1f);
            card_game_position_hand(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), HAND_SPACING, -HAND_FAN_ANGLE, HAND_CURVE_AMOUNT, HAND_POSITION_OFFSET);
         }
        gs_dyn_array_erase(card_game->player_hand, card_game->hovered_index );
        card_game->phase = DO_ON_PLAY_EFFECTS;

    } else if (card_game->phase == DO_ON_PLAY_EFFECTS) {
        if (card_game->player_card_played || card_game->opponent_card_played) {
            card_game_do_card_played(card_game, game_state);
            card_game_position_hand(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), HAND_SPACING, HAND_FAN_ANGLE, -HAND_CURVE_AMOUNT, -HAND_POSITION_OFFSET);
            card_game_position_hand(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), HAND_SPACING, -HAND_FAN_ANGLE, HAND_CURVE_AMOUNT, HAND_POSITION_OFFSET);
        }
         card_game->phase = PLAYER_SELECT_TARGET;
    } else if (card_game->phase == PLAYER_SELECT_TARGET) {
        if (card_game->player_card_played) {
            if (card_game->player_in_play_card.abilities.strike > 0) {
                card_game->player_selecting_target = true;
            }
            card_game->player_card_played = false;
        } else if (card_game->player_selecting_target) {
            card_game_set_player_targets_selectable(card_game, game_state);
            if (card_game->hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
                card_state_t *target;
                if (card_game->hovered_index < 10) {
                    target = &card_game->player_hand[card_game->hovered_index];
                } else if (card_game->hovered_index < 20) {
                    target = &card_game->opponent_hand[card_game->hovered_index - 10];
                } else if (card_game->hovered_index == 20) {
                    target = &card_game->player_in_play_card;
                } else {
                    target = &card_game->opponent_in_play_card;
                }
                printf("target: %s\n", target->name);
                if (target->current_abilities.shield > 0) {
                    target->current_abilities.shield--;
                } else {
                    target->current_health -= card_game->player_in_play_card.abilities.strike;
                }
                card_game->visual_update = true;
                card_game->player_selecting_target = false;
                card_game_resolve_damage(card_game, game_state);
            }
        } else if (!card_game->player_selecting_target) { // wait while player is selecting target
            card_game->phase = OPPONENT_SELECT_TARGET;
        }
    } else if (card_game->phase == OPPONENT_SELECT_TARGET) {
        if (card_game->opponent_card_played) {
            card_game->opponent_card_played = false;
        }
        card_game->phase = BATTLE;
        card_game->battle_timer = 0.f;
    } else if (card_game->phase == BATTLE) {

        float prev_battle_timer = card_game->battle_timer;
        card_game->battle_timer += dt;
        float tick_resolution = 2.0;
        float haste_time = 1;
        float haste_attack_anim_time = 0.5;
        float haste_end_attack_anim_time = 0.7;
        float attack_anim_time = 1.5;
        float end_attack_anim_time = 1.7; // must be greater than attack_anim_time and less that tick_resolution

        // haste animation timer ticks
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, haste_attack_anim_time, tick_resolution)) {
            printf("haste anim: %f\n", card_game->battle_timer);
            card_game->player_card_attacking = card_game->player_in_play_card.abilities.haste;
            card_game->opponent_card_attacking = card_game->opponent_in_play_card.abilities.haste;
            if (card_game->player_card_attacking) {
                card_game_set_animation(&card_game->player_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(-1.5f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.07f);
            }
            if (card_game->opponent_card_attacking) {
                card_game_set_animation(&card_game->opponent_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(1.5f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.07f);
            }
        }

        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, haste_end_attack_anim_time, tick_resolution)) {
            printf("haste anim: %f\n", card_game->battle_timer);
            if (card_game->player_card_attacking) {
                card_game_set_animation(&card_game->player_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(-2.f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.1f);
            }
            if (card_game->opponent_card_attacking) {
                card_game_set_animation(&card_game->opponent_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(2.f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.1f);
            }
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            if (card_game->player_in_play_card.abilities.haste) {
                card_game_do_player_card_attack_opponent(card_game, game_state);
            }
            if (card_game->opponent_in_play_card.abilities.haste) {
                card_game_do_opponent_card_attack_player(card_game, game_state);
            }
        }
        // haste battle tick
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, haste_time, tick_resolution)) {
            printf("haste battle tick: %f\n", card_game->battle_timer);
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            card_game_resolve_damage(card_game, game_state);
            card_game_position_hand(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), HAND_SPACING, HAND_FAN_ANGLE, -HAND_CURVE_AMOUNT, -HAND_POSITION_OFFSET);
            card_game_position_hand(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), HAND_SPACING, -HAND_FAN_ANGLE, HAND_CURVE_AMOUNT, HAND_POSITION_OFFSET);
        }

        // normal animation timer ticks
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, attack_anim_time, tick_resolution)) {
            printf("anim: %f\n", card_game->battle_timer);
            card_game->player_card_attacking = !card_game->player_in_play_card.abilities.haste;
            card_game->opponent_card_attacking = !card_game->opponent_in_play_card.abilities.haste;
            if (card_game->player_card_attacking) {
                card_game_set_animation(&card_game->player_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(-1.5f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.07f);
            }
            if (card_game->opponent_card_attacking) {
                card_game_set_animation(&card_game->opponent_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(1.5f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.07f);
            }
        }
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, end_attack_anim_time, tick_resolution)) {
            printf("anim: %f\n", card_game->battle_timer);
            if (card_game->player_card_attacking) {
                card_game_set_animation(&card_game->player_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(-2.f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.1f);
            }
            if (card_game->opponent_card_attacking) {
                card_game_set_animation(&card_game->opponent_in_play_card,
                    (gs_vqs_t) {
                        .position = gs_v3(2.f, 0.f, 0.f),
                        .rotation = gs_quat_default(),
                        .scale = gs_v3(1.f, 1.f, 1.f)
                    },
                    0.1f);
            }
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            if (!card_game->player_in_play_card.abilities.haste) {
                card_game_do_player_card_attack_opponent(card_game, game_state);
            }
            if (!card_game->opponent_in_play_card.abilities.haste) {
                card_game_do_opponent_card_attack_player(card_game, game_state);
            }
        }
        // normal battle tick
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, 0, tick_resolution)) {
            printf("battle tick: %f\n", card_game->battle_timer);
            card_game->player_card_attacking = false;
            card_game->opponent_card_attacking = false;
            card_game_resolve_damage(card_game, game_state);
            card_game_position_hand(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), HAND_SPACING, HAND_FAN_ANGLE, -HAND_CURVE_AMOUNT, -HAND_POSITION_OFFSET);
            card_game_position_hand(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), HAND_SPACING, -HAND_FAN_ANGLE, HAND_CURVE_AMOUNT, HAND_POSITION_OFFSET);
        }
    }

    if (card_game->visual_update) {
        card_game_update_all_cards(card_game, &game_state->immediate_draw);
    }
    card_game_set_hovered_card(card_game, game_state);
    card_game_animate_card_transforms(card_game, dt);

    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&game_state->camera, (int32_t)fbw, (int32_t)fbh);
    // NOTE: cards are drawn depth wise in order, so sort based on draw order before rendering
    // In that same spirit, we should iterate backwards over the cards to check if any are hovered
    cards_render_instanced(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), &game_state->command_buffer, view_projection);
    cards_render_instanced(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), &game_state->command_buffer, view_projection);
    // if there is a player card in play, position and rotate correctly
    if (card_game->player_in_play_card.name != NULL) {
        cards_render_instanced(&card_game->player_in_play_card, 1, &game_state->command_buffer, view_projection);
    }
    // if there is a opponent card in play, position and rotate correctly
    if (card_game->opponent_in_play_card.name != NULL) {
        cards_render_instanced(&card_game->opponent_in_play_card, 1, &game_state->command_buffer, view_projection);
    }
}
