#define GS_IMPL
#define GS_IMMEDIATE_DRAW_IMPL
#define GS_GUI_IMPL
#include "gs.h"

#include "util_game.h"
#include "util_card_game.h"
#include "util_gui.h"


static game_state_t state = {0};
static card_game_state_t card_game = {0};

void init() {
    srand(time(NULL));
    game_state_init(&state);
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        state.mode = MENU;
    }

    gs_gui_begin(&state.gui_ctx, NULL);
    if (state.mode == MENU) {
        state.mode = gui_show_menu(&state);
        if (state.mode == GAME) {
            card_game.simulate_player = true;
            card_game.game_speed = 60.0f;
            card_game.simulation_count = 1000;
            card_game.player_wins = 0;
            card_game.opponent_wins = 0;
            card_game.draws = 0;
            for (int i = 0; i < 100; i++) card_game.winning_card_counts[i] = 0;
            card_game_init(&card_game, &state);
        }
    }
    gs_gui_end(&state.gui_ctx);

    game_render_begin(&state);
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
