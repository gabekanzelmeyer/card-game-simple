#define GS_IMPL
#include "gs.h"

#define GS_IMMEDIATE_DRAW_IMPL
#include "util/gs_idraw.h"

#define GS_GUI_IMPL
#include "util/gs_gui.h"

#include "cards.h"

#define HOVER_SCREEN_SIZE 300

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

enum game_state {
    MENU,
    GAME
};

enum card_game_state {
    SELECT_CARD
};


typedef struct {
    gs_command_buffer_t command_buffer;
    gs_immediate_draw_t immediate_draw;
    gs_gui_context_t gui_ctx;
    gs_camera_t camera;
    enum game_state current_state;
    enum card_game_state current_card_game_state;
    gs_dyn_array(card_state) player_hand;
    gs_dyn_array(card_state) opponent_hand;
    card_state *player_in_play_card;
    card_state *opponent_in_play_card;
} game_state;

static game_state state = {0};


void position_hand(card_state *cards, int count, float spacing, float tilt, float curve_amount, float y_offset) {
    float start_x = -spacing * (float)count / 2.f + spacing / 2.f;
    float start_tilt = tilt * (float)count / 2.f - tilt / 2.f;
    for (int i = 0; i < count; i++) {
        cards[i].transform.position.x = start_x + i * spacing;
        cards[i].transform.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * curve_amount * (1.f / spacing) + y_offset;
        cards[i].transform.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * tilt)), gs_v3(0, 0, 1));
        if (curve_amount > 0) {
            cards[i].transform.rotation = gs_quat_mul(
                gs_quat_angle_axis(gs_deg2rad(180), gs_v3(0, 0, 1)),
                cards[i].transform.rotation);
        }
    }
}

void init_card_game() {
    int instance_index = 0;

    gs_dyn_array_free(state.player_hand);
    gs_dyn_array_free(state.opponent_hand);
    for (int i = 0; i < 6; i++) {
        gs_dyn_array_push(state.player_hand, card_new("CARD NAME", 3, 3, instance_index++));
        gs_dyn_array_push(state.opponent_hand, card_new("CARD NAME", 3, 3, instance_index++));
    }

    state.player_in_play_card = NULL;
    state.opponent_in_play_card = NULL;

    state.current_state = GAME;
    state.current_card_game_state = SELECT_CARD;

    for (int i = 0; i < gs_dyn_array_size(state.player_hand); ++i) {
        card_bake_texture(&state.player_hand[i], &state.immediate_draw);
        card_bake_texture(&state.opponent_hand[i], &state.immediate_draw);
    }

    position_hand(state.player_hand, gs_dyn_array_size(state.player_hand), 0.9f, 10.0f, -0.08, -2.5);
    position_hand(state.opponent_hand, gs_dyn_array_size(state.opponent_hand), 0.9f, -10.0f, 0.08,  2.5);
}

void init() {
    gs_gui_init(&state.gui_ctx, gs_platform_main_window());

    state.command_buffer = gs_command_buffer_new();
    state.immediate_draw = gs_immediate_draw_new();
    state.camera = gs_camera_perspective();
    state.camera .transform.position = gs_v3(0.f, 0.f, 6.f);

    card_global_init();

    gs_gui_style_element_t font_style[] = {{ .type = GS_GUI_STYLE_FONT, .font = &cards_global.card_font}};

    gs_gui_set_element_style(&state.gui_ctx, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_DEFAULT, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state.gui_ctx, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_HOVER, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state.gui_ctx, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_FOCUS, font_style, sizeof(font_style));

    init_card_game();

    state.current_state = MENU;
}

bool world_to_screen(gs_vec3 world_pos, gs_vec2* out_screen, gs_mat4 view_proj, float screen_width, float screen_height) {
    float x = world_pos.x * view_proj.m[0][0] + world_pos.y * view_proj.m[1][0] + world_pos.z * view_proj.m[2][0] + view_proj.m[3][0];
    float y = world_pos.x * view_proj.m[0][1] + world_pos.y * view_proj.m[1][1] + world_pos.z * view_proj.m[2][1] + view_proj.m[3][1];
    float z = world_pos.x * view_proj.m[0][2] + world_pos.y * view_proj.m[1][2] + world_pos.z * view_proj.m[2][2] + view_proj.m[3][2];
    float w = world_pos.x * view_proj.m[0][3] + world_pos.y * view_proj.m[1][3] + world_pos.z * view_proj.m[2][3] + view_proj.m[3][3];

    // behind the camera if w is less than or equal to 0
    if (w <= 0.0f) {
        return false;
    }

    // calculate normalized device coordinates
    float ndc_x = x / w;
    float ndc_y = y / w;

    // caculate screen coords
    out_screen->x = ((ndc_x + 1.0f) * 0.5f) * screen_width;
    out_screen->y = ((1.0f - ndc_y) * 0.5f) * screen_height; // Inverted Y for standard 2D screen coordinate spaces

    return true;
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        state.current_state = MENU;
    }

    gs_gui_begin(&state.gui_ctx, NULL);
    if (state.current_state == MENU) {
        if (gs_gui_window_begin_ex(&state.gui_ctx, "main", gs_gui_rect(0, 0, 0, 0), NULL, NULL,GS_GUI_OPT_NOTITLE
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
                &state.gui_ctx.viewport, // parent rect to center within (full screen viewport)
                500, 300, // button size
                0, 0, // x/y offset from the centered position
                GS_GUI_LAYOUT_ANCHOR_CENTER
            );
            gs_gui_layout_set_next(&state.gui_ctx, centered, 0);
            if (gs_gui_button(&state.gui_ctx, "Play")) {
                init_card_game();
            }
        }
        gs_gui_window_end(&state.gui_ctx);
    }
    gs_gui_end(&state.gui_ctx);

    float dt = gs_platform_delta_time();
    gs_vec2 mouse_pos = gs_platform_mouse_positionv();

    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);

    gs_graphics_clear_action_t clear = {
        .flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH,
        .color = {0.1f, 0.1f, 0.15f, 1.f}
    };
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(&state.command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT); // default (screen) render pass
    gs_graphics_clear(&state.command_buffer, &clear_desc);
    gs_graphics_set_viewport(&state.command_buffer, 0, 0, fbw, fbh);

    if (state.current_state == GAME) {
        gs_mat4 view_projection = gs_camera_get_view_projection(&state.camera, (int32_t)fbw, (int32_t)fbh);

        // NOTE: cards are drawn depth wise in order, so sort based on draw order before rendering
        // In that same spirit, we should iterate backwards over the cards to check if any are hovered
        for (int i = gs_dyn_array_size(state.player_hand) - 1; i >= 0; i--) {
            state.player_hand[i].transform.scale = gs_v3(1.f, 1.f, 1.f);
        }

        int hovered_index = -1;
        for (int i = gs_dyn_array_size(state.player_hand) - 1; i >= 0; i--) {
            gs_vec2 screen_pos;
            world_to_screen(state.player_hand[i].transform.position, &screen_pos, view_projection, fbw, fbh);
            if (gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < HOVER_SCREEN_SIZE)  {
                state.player_hand[i].transform.scale = gs_v3(1.2f, 1.2f, 1.2f);
                hovered_index = i;
                break;
            }
        }
        card_render_instanced(state.player_hand, gs_dyn_array_size(state.player_hand), &state.command_buffer, view_projection);
        card_render_instanced(state.opponent_hand, gs_dyn_array_size(state.opponent_hand), &state.command_buffer, view_projection);

        if (hovered_index != -1 && gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
            printf("selected: %s\n", state.player_hand[hovered_index].name);
            gs_dyn_array_erase(state.player_hand, hovered_index);
            position_hand(state.player_hand, gs_dyn_array_size(state.player_hand), 0.9f, 10.0f, -0.08, -2.5);
        }
    }

    gs_gui_render(&state.gui_ctx, &state.command_buffer);

    gs_graphics_renderpass_end(&state.command_buffer);
    gs_graphics_command_buffer_submit(&state.command_buffer);
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
