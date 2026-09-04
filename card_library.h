#ifndef CARD_LIBRARY_H
#define CARD_LIBRARY_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "card_data.h"
#include "card_database.h"
#include "card_renderer.h"
#include "game_util.h"

static gs_dyn_array(card_state_t) card_library;

void card_library_init() {
    gs_dyn_array_free(card_library);

    int render_index = 0;
    for (int i = 0; i < gs_dyn_array_size(card_database); i++) {
        card_state_t card = card_database[i];
        card.render_index = render_index++;
        gs_dyn_array_push(card_library, card);
    }
}

void card_library_update(game_state_t *game_state) {
    for (int i = gs_dyn_array_size(card_library) - 1; i >= 0; i--) {
        gs_vqs_t transform = gs_vqs_default();
        transform.position.x = -5 + (i % 6) * 1.5;
        transform.position.y = 2 - (i / 6) * 2;
        card_library[i].transform = transform;
        card_update_visuals(game_state->card_renderer, &card_library[i], &game_state->immediate_draw);
    }

    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&game_state->camera, (int32_t)fbw, (int32_t)fbh);
    card_render_instanced(game_state->card_renderer, card_library, gs_dyn_array_size(card_library), &game_state->command_buffer, view_projection);
}

#endif
