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
    SELECT_CARD,
    OPPONENT_SELECT_CARD,
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
    bool player_card_attacking;
    bool opponent_card_attacking;
} card_game_state_t;

void card_game_position_hand(card_state_t *cards, int count, float spacing, float tilt, float curve_amount, float y_offset) {
    float start_x = -spacing * (float)count / 2.f + spacing / 2.f;
    float start_tilt = tilt * (float)count / 2.f - tilt / 2.f;
    for (int i = 0; i < count; i++) {
        cards[i].transform.position.x = start_x + i * spacing;
        cards[i].transform.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * curve_amount * (1.f / spacing) + y_offset;
        cards[i].transform.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * tilt)), gs_v3(0, 0, 1));
    }
}

void card_game_update_all_cards(card_game_state_t *card_game, gs_immediate_draw_t *immediate_draw) {
    for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); ++i) {
        card_update(&card_game->player_hand[i], immediate_draw);
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); ++i) {
        card_update(&card_game->opponent_hand[i], immediate_draw);
    }
    if (card_game->player_in_play_card.name != NULL) {
        card_update(&card_game->player_in_play_card, immediate_draw);
    }
    if (card_game->opponent_in_play_card.name != NULL) {
        card_update(&card_game->opponent_in_play_card, immediate_draw);
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

    card_game->phase = SELECT_CARD;

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
        }
    }
    for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
        if (card_game->opponent_hand[i].current_health <= 0) {
            gs_dyn_array_erase(card_game->opponent_hand, i);
            was_a_card_destroyed = true;
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
                card_game->phase = SELECT_CARD;
            } else {
                card_game->phase = OPPONENT_SELECT_CARD;
            }
        }
    }
}


void card_game_do_card_played(card_game_state_t *card_game, game_state_t *game_state) {
    if (card_game->player_card_played) {
        if (card_game->player_in_play_card.abilities.strike > 0) {
            if (card_game->opponent_in_play_card.current_abilities.shield > 0) {
                card_game->opponent_in_play_card.current_abilities.shield--;
            } else {
                card_game->opponent_in_play_card.current_health -= card_game->player_in_play_card.abilities.strike;
            }
        }
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
        if (card_game->opponent_in_play_card.abilities.strike > 0) {
            if (card_game->player_in_play_card.current_abilities.shield > 0) {
                card_game->player_in_play_card.current_abilities.shield--;
            } else {
                card_game->player_in_play_card.current_health -= card_game->opponent_in_play_card.abilities.strike;
            }
        }
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

static bool card_game_battle_timer_trigger_happens(card_game_state_t *card_game, float prev_timer, float current_timer, float trigger, float resolution) {
    if (floorf((prev_timer - trigger) / resolution) * resolution
        < floorf((current_timer - trigger) / resolution) * resolution) {
        return true;
    }
    return false;
}

void card_game_update(card_game_state_t *card_game, game_state_t *game_state) {
    float dt = gs_platform_delta_time();
    gs_vec2 mouse_pos = gs_platform_mouse_positionv();
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&game_state->camera, (int32_t)fbw, (int32_t)fbh);

    // reset sizes of player hand cards
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        card_game->player_hand[i].transform.scale = gs_v3(1.f, 1.f, 1.f);
    }

    // increase scale of hovered hand card and keep track of which card for later
    int hovered_index = -1;
    for (int i = gs_dyn_array_size(card_game->player_hand) - 1; i >= 0; i--) {
        gs_vec2 screen_pos;
        world_to_screen(card_game->player_hand[i].transform.position, &screen_pos, view_projection, fbw, fbh);
        if (gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE)  {
            card_game->player_hand[i].transform.scale = gs_v3(1.2f, 1.2f, 1.2f);
            hovered_index = i;
            break;
        }
    }

    // NOTE: cards are drawn depth wise in order, so sort based on draw order before rendering
    // In that same spirit, we should iterate backwards over the cards to check if any are hovered
    card_game_position_hand(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), HAND_SPACING, HAND_FAN_ANGLE, -HAND_CURVE_AMOUNT, -HAND_POSITION_OFFSET);
    card_game_position_hand(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), HAND_SPACING, -HAND_FAN_ANGLE, HAND_CURVE_AMOUNT, HAND_POSITION_OFFSET);

    cards_render_instanced(card_game->player_hand, gs_dyn_array_size(card_game->player_hand), &game_state->command_buffer, view_projection);
    cards_render_instanced(card_game->opponent_hand, gs_dyn_array_size(card_game->opponent_hand), &game_state->command_buffer, view_projection);

    // if there is a player card in play, position and rotate correctly
    if (card_game->player_in_play_card.name != NULL) {
        card_game->player_in_play_card.transform.position = gs_v3(card_game->player_card_attacking ? -1.5f : -2., 0.f, 0.f);
        card_game->player_in_play_card.transform.rotation = gs_quat_default();
        card_game->player_in_play_card.transform.scale = gs_v3(1.f, 1.f, 1.f);
        cards_render_instanced(&card_game->player_in_play_card, 1, &game_state->command_buffer, view_projection);
    }
    // if there is a opponent card in play, position and rotate correctly
    if (card_game->opponent_in_play_card.name != NULL) {
        card_game->opponent_in_play_card.transform.position = gs_v3(card_game->opponent_card_attacking ? 1.5f : 2., 0.f, 0.f);
        card_game->opponent_in_play_card.transform.rotation = gs_quat_default();
        card_game->opponent_in_play_card.transform.scale = gs_v3(1.f, 1.f, 1.f);
        cards_render_instanced(&card_game->opponent_in_play_card, 1, &game_state->command_buffer, view_projection);
    }

    if (card_game->phase == SELECT_CARD) {
        if (hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
            // if there are timebound hands in the players hand, one of them must be played
            bool has_timebound = false;
            for (int i = 0; i < gs_dyn_array_size(card_game->player_hand); i++) {
                if (card_game->player_hand[i].abilities.timebound) has_timebound = true;
            }
            if (!has_timebound || card_game->player_hand[hovered_index].abilities.timebound) {
                card_game->player_in_play_card = card_game->player_hand[hovered_index];
                card_game->player_card_played = true;
                gs_dyn_array_erase(card_game->player_hand, hovered_index);

                // once we set the plater "in play" card, see if there is an opponent "in play" card,
                // if so, move to the battle stage. otherwise, move to the opponent play card stage
                if (card_game->opponent_in_play_card.name == NULL) {
                    card_game->phase = OPPONENT_SELECT_CARD;
                } else {
                    card_game->phase = BATTLE;
                    card_game->battle_timer = 0.f;
                }
            }
        }
    } else if (card_game->phase == OPPONENT_SELECT_CARD) {
        // if there are timebound hands in the opponents hand, one of them must be played
        // gather timebound indices
        gs_dyn_array(int) timebound_indices = NULL;
        for (int i = 0; i < gs_dyn_array_size(card_game->opponent_hand); i++) {
            if (card_game->opponent_hand[i].abilities.timebound) gs_dyn_array_push(timebound_indices, i);
        }

        // if there are timebound cards, grab a random one of those
        int random_index;
        printf("size : %i", gs_dyn_array_size(timebound_indices));
        if (gs_dyn_array_size(timebound_indices) > 0) {
            random_index = timebound_indices[(rand() % gs_dyn_array_size(timebound_indices))];
        } else {
            random_index = (rand() % gs_dyn_array_size(card_game->opponent_hand));
        }
        printf("random : %i", random_index);

        card_game->opponent_in_play_card = card_game->opponent_hand[random_index];
        card_game->opponent_card_played = true;
        gs_dyn_array_erase(card_game->opponent_hand, random_index);
        card_game->phase = BATTLE;
        card_game->battle_timer = 0.f;
    } else if (card_game->phase == BATTLE) {
        if (card_game->player_card_played || card_game->opponent_card_played) {
            card_game_do_card_played(card_game, game_state);
            card_game->player_card_played = false;
            card_game->opponent_card_played = false;
        }

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
        }

        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, haste_end_attack_anim_time, tick_resolution)) {
            printf("haste anim: %f\n", card_game->battle_timer);
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
        }

        // normal animation timer ticks
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, attack_anim_time, tick_resolution)) {
            printf("anim: %f\n", card_game->battle_timer);
            card_game->player_card_attacking = !card_game->player_in_play_card.abilities.haste;
            card_game->opponent_card_attacking = !card_game->opponent_in_play_card.abilities.haste;
        }
        if (card_game_battle_timer_trigger_happens(card_game, prev_battle_timer, card_game->battle_timer, end_attack_anim_time, tick_resolution)) {
            printf("anim: %f\n", card_game->battle_timer);
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
        }
    }
}
