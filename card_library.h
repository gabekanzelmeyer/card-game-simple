#ifndef CARD_LIBRARY_H
#define CARD_LIBRARY_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "card_data.h"
#include "card_database.h"
#include "card_renderer.h"
#include "game_util.h"

#define ROWS_PER_PAGE 2
#define CARDS_PER_PAGE 12

static gs_dyn_array(card_state_t) card_library;
static gs_dyn_array(card_state_t) card_library_view;
static gs_dyn_array(card_state_t) card_library_hand;

static int num_pages = 1;
static int current_page_index = 0;
static int prev_page = -1;
int32_t card_library_red_enabled = false;
int32_t card_library_green_enabled = false;
int32_t card_library_blue_enabled = false;

void card_library_init() {
    gs_dyn_array_free(card_library);
    gs_dyn_array_free(card_library_view);
    gs_dyn_array_free(card_library_hand);

    for (int i = 0; i < gs_dyn_array_size(card_database); i++) {
        gs_dyn_array_push(card_library, card_database[i]);
    }

    num_pages = (gs_dyn_array_size(card_library) / CARDS_PER_PAGE) + 1;
    current_page_index = 0;
}

static void card_library_filter_view(game_state_t *game_state, int page_index) {
    gs_dyn_array_free(card_library_view);

    // first thing is filter card database based on filters
    gs_dyn_array(card_state_t) filtered_cards = NULL;
    int32_t all_disabled = !(card_library_red_enabled || card_library_green_enabled || card_library_blue_enabled);
    for (int i = 0; i < gs_dyn_array_size(card_database); i++) {
        if (card_database[i].red && (card_library_red_enabled || all_disabled)) gs_dyn_array_push(filtered_cards, card_database[i]);
        else if (card_database[i].green && (card_library_green_enabled || all_disabled)) gs_dyn_array_push(filtered_cards, card_database[i]);
        else if (card_database[i].blue && (card_library_blue_enabled || all_disabled))  gs_dyn_array_push(filtered_cards, card_database[i]);
    }
    current_page_index = page_index;

    int page_start_index = current_page_index * CARDS_PER_PAGE;
    int page_end_index = fmin(gs_dyn_array_size(filtered_cards) - 1, page_start_index + CARDS_PER_PAGE - 1);
    int render_index = 0;
    for (int i = page_end_index; i >= page_start_index; i--) {
        gs_vqs_t transform = gs_vqs_default();
        transform.position.x = -4.25 + (i % 6) * 1.75;
        transform.position.y = 2 - ((i - page_start_index) / 6) * 2.25;
        card_state_t card = filtered_cards[i];
        card.transform = transform;
        card.render_index = render_index++;
        gs_dyn_array_push(card_library_view, card);
        card_update_visuals(game_state->card_renderer, &card, &game_state->immediate_draw);
    }

     gs_dyn_array_free(filtered_cards);
}

enum game_mode card_library_gui(game_state_t *state) {
    enum game_mode result = LIBRARY;
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

            char page_count_text[64];
            snprintf(page_count_text, sizeof(page_count_text), "Page: %i/%i", (current_page_index + 1), num_pages);
            gs_gui_rect_t page_count_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport,400, 200, -300, 0, GS_GUI_LAYOUT_ANCHOR_TOPCENTER);
            gs_gui_layout_set_next(&state->gui_ctx, page_count_rect, 0);
            gs_gui_text(&state->gui_ctx, page_count_text);

            gs_gui_rect_t prev_button_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport,200, 200, 0, 0, GS_GUI_LAYOUT_ANCHOR_TOPCENTER);
            gs_gui_layout_set_next(&state->gui_ctx, prev_button_rect, 0);
            if (gs_gui_button(&state->gui_ctx, "<<")) {
                current_page_index = current_page_index - 1;
                if (current_page_index < 0) current_page_index = num_pages - 1;
            }
            gs_gui_rect_t next_button_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport,200, 200, 220, 0, GS_GUI_LAYOUT_ANCHOR_TOPCENTER);
            gs_gui_layout_set_next(&state->gui_ctx, next_button_rect, 0);
            if (gs_gui_button(&state->gui_ctx, ">>")) {
                current_page_index = (current_page_index + 1) % num_pages;
            }

            gs_gui_rect_t red_checkbox_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport, 400, 100, -600, 30, GS_GUI_LAYOUT_ANCHOR_TOPRIGHT);
            gs_gui_layout_set_next(&state->gui_ctx, red_checkbox_rect, 0);
            if (gs_gui_checkbox(&state->gui_ctx, "Red", &card_library_red_enabled)) {
                card_library_filter_view(state, 0);
            }

            gs_gui_rect_t green_checkbox_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport, 400, 100, -300, 30, GS_GUI_LAYOUT_ANCHOR_TOPRIGHT);
            gs_gui_layout_set_next(&state->gui_ctx, green_checkbox_rect, 0);
            if (gs_gui_checkbox(&state->gui_ctx, "Green", &card_library_green_enabled)) {
                card_library_filter_view(state, 0);
            }

            gs_gui_rect_t blue_checkbox_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport, 400, 100, 100, 30, GS_GUI_LAYOUT_ANCHOR_TOPRIGHT);
            gs_gui_layout_set_next(&state->gui_ctx, blue_checkbox_rect, 0);
            if (gs_gui_checkbox(&state->gui_ctx, "Blue", &card_library_blue_enabled)) {
                card_library_filter_view(state, 0);
            }

            gs_gui_rect_t play_button_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport, 400, 200, 0, 0, GS_GUI_LAYOUT_ANCHOR_BOTTOMRIGHT);
            gs_gui_layout_set_next(&state->gui_ctx, play_button_rect, 0);
            if (gs_gui_button(&state->gui_ctx, "Play")) {
                if (gs_dyn_array_size(card_library_hand) == 6) {
                    result = GAME;
                }
            }
        }
        gs_gui_window_end(&state->gui_ctx);
        return result;

}

void card_library_update(game_state_t *game_state) {
    if (prev_page != current_page_index) {
        card_library_filter_view(game_state, current_page_index);
        prev_page = current_page_index;
    }

    gs_vec2 mouse_pos = gs_platform_mouse_positionv();
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&game_state->camera, (int32_t)fbw, (int32_t)fbh);
    gs_vec2 screen_pos;

    int hovered_index = -1;
    bool added = false;

    for (int i = gs_dyn_array_size(card_library_view) - 1; i >= 0; i--) {
        world_to_screen(card_library_view[i].transform.position, &screen_pos, view_projection, fbw, fbh);
        if (gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < 300
            && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)
            && gs_dyn_array_size(card_library_hand) < 6
            && !hand_contains_card(card_library_hand, card_library_view[i])) {

            added = true;
            gs_dyn_array_push(card_library_hand, card_library_view[i]);
            position_hand_cards(card_library_hand, true);

            int render_index = CARDS_PER_PAGE;
            for (int i = gs_dyn_array_size(card_library_hand) - 1; i >= 0; i--) {
                card_library_hand[i].render_index = render_index++;
                card_update_visuals(game_state->card_renderer, &card_library_hand[i], &game_state->immediate_draw);
            }

            break;
        }
    }
    for (int i = gs_dyn_array_size(card_library_hand) - 1; i >= 0; i--) {
        world_to_screen(card_library_hand[i].transform.position, &screen_pos, view_projection, fbw, fbh);
        if (gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < 300
            && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)
            && !added) {

            gs_dyn_array_erase(card_library_hand, i);
            position_hand_cards(card_library_hand, true);

            int render_index = CARDS_PER_PAGE;
            for (int i = gs_dyn_array_size(card_library_hand) - 1; i >= 0; i--) {
                card_library_hand[i].render_index = render_index++;
                card_update_visuals(game_state->card_renderer, &card_library_hand[i], &game_state->immediate_draw);
            }

            break;
        }
    }

    float dt = gs_platform_delta_time();
    for (int i = 0; i < gs_dyn_array_size(card_library_hand); i++) {
        lerp_card_transform(&card_library_hand[i], dt);
    }
    card_render_instanced(game_state->card_renderer, card_library_view, gs_dyn_array_size(card_library_view), &game_state->command_buffer, view_projection);
    card_render_instanced(game_state->card_renderer, card_library_hand, gs_dyn_array_size(card_library_hand), &game_state->command_buffer, view_projection);
}

#endif
