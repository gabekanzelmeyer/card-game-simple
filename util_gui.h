#ifndef UTIL_GUI_H
#define UTIL_GUI_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "util_game.h"

enum game_mode gui_show_menu(game_state_t *state) {
    enum game_mode result = MENU;
    if (gs_gui_window_begin_ex(&state->gui_ctx, "main", gs_gui_rect(0, 0, 0, 0), NULL, NULL,GS_GUI_OPT_NOTITLE
        | GS_GUI_OPT_NORESIZE
        | GS_GUI_OPT_NOMOVE
        | GS_GUI_OPT_NOSCROLL
        | GS_GUI_OPT_NOCLOSE
        | GS_GUI_OPT_NOFRAME
        | GS_GUI_OPT_NOSTYLEBORDER
        | GS_GUI_OPT_NOSTYLESHADOW
        | GS_GUI_OPT_NOSTYLEBACKGROUND
        | GS_GUI_OPT_FULLSCREEN)) {
        gs_gui_rect_t centered = gs_gui_layout_anchor(
            &state->gui_ctx.viewport, // parent rect to center within (full screen viewport)
            500, 300, // button size
            0, 0, // x/y offset from the centered position
            GS_GUI_LAYOUT_ANCHOR_CENTER
        );
        gs_gui_layout_set_next(&state->gui_ctx, centered, 0);
        if (gs_gui_button(&state->gui_ctx, "Play")) {
            result = GAME;
        }
    }
    gs_gui_window_end(&state->gui_ctx);
    return result;
}

#endif
