#define GS_IMPL
#define GS_IMMEDIATE_DRAW_IMPL
#define GS_GUI_IMPL
#include "gs.h"

#include "card_renderer.h"
#include "card_database.h"
#include "card_library.h"
#include "card_game.h"
#include "game_util.h"

static game_state_t state = {0};
static card_game_state_t card_game = {0};
static card_render_data_t card_renderer = {0};

void init() {
    srand(time(NULL));
    card_renderer_init(&card_renderer);
    card_database_init();
    game_state_init(&state, &card_renderer);
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        state.mode = MENU;
    }

    gs_gui_begin(&state.gui_ctx, NULL);
    if (state.mode == MENU) {
        state.mode = gui_show_menu(&state);
        if (state.mode == LIBRARY) {
            card_library_init();
        }
        if (state.mode == GAME) {
            card_game = (card_game_state_t){0};
            card_game.simulate_player = true;
            card_game.game_speed = 500.0f;
            card_game.simulation_count = 2000;
            card_game.simulate_color_v_color = false;
            for (int i = 0; i < 100; i++) card_game.winning_card_counts[i] = 0;

            gs_dyn_array(card_state_t) player_hand = card_game.simulate_color_v_color ? hand_get_random_color() : hand_get_random(true, true, true);
            gs_dyn_array(card_state_t) opponent_hand = card_game.simulate_color_v_color ? hand_get_random_color() : hand_get_random(true, true, true);
            card_game_init(&card_game, &state, player_hand, opponent_hand);
        }
    } else if (state.mode == LIBRARY) {
        state.mode = card_library_gui(&state);
        if (state.mode == GAME) {
            card_game = (card_game_state_t){0};
            card_game.game_speed = 1.0f;
            gs_dyn_array(card_state_t) opponent_hand = hand_get_random(card_library_red_enabled, card_library_green_enabled, card_library_blue_enabled);
            card_game_init(&card_game, &state, card_library_hand, opponent_hand);
        }
    } else if (state.mode == GAME && card_game.simulate_player) {
        card_game_show_simulation_gui(&card_game, &state);
    }
    gs_gui_end(&state.gui_ctx);

    game_render_begin(&state);
    if (state.mode == LIBRARY) {
        card_library_update(&state);
    }
    if (state.mode == GAME) {
        card_game_update(&card_game, &state);
    }
    gs_gui_render(&state.gui_ctx, &state.command_buffer);
    game_render_end(&state);
}

gs_app_desc_t gs_main(int32_t argc, char** argv) {
    return (gs_app_desc_t){
        .init = init,
        .update = update,
        .window = {
            .title = "Card Game",
            .width = 1600,
            .height = 1000
        }
    };
}
