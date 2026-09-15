#define GS_IMPL
#define GS_IMMEDIATE_DRAW_IMPL
#define GS_GUI_IMPL

#include "gs.h"

#include "engine.h"
#include "engine_shapes.h"
#include "card_entity.h"
#include "card_database.h"
#include "card_library.h"

// #include "card_renderer.h"
// #include "card_database.h"
// #include "card_game.h"
// #include "game_util.h"

// static game_state_t state = {0};
// static card_game_state_t card_game = {0};
// static card_render_data_t card_renderer = {0};

gs_vec3 player_pos;
float player_speed = 6.f;
gs_vec3 camera_offset;
static engine_t engine;
static gs_asset_font_t font;
static gs_handle(gs_graphics_texture_t) card_bg_texture;
entity_t sphere;
entity_t plane;
card_entity_t card;
gs_dyn_array(card_entity_t) hand;


void init() {
    srand(time(NULL));
    // card_renderer_init(&card_renderer);
    // game_state_init(&state, &card_renderer);

    camera_offset = gs_v3(0.f, 18.f, 10.f);
    player_pos = gs_v3(0.f, 0.5f, 0.f);

    engine_init(&engine);
    card_database_init();
    card_entites_init();
    card_library_init();

    engine.camera.transform.position = gs_vec3_add(player_pos, camera_offset);
    engine.camera.transform.rotation = gs_quat_angle_axis(gs_deg2rad(-60.f), gs_v3(1.f, 0.f, 0.f));

    sphere.mesh = mesh_sphere(0.5f, 16, 24);
    sphere.transform = gs_vqs_default();
    sphere.material.texture = NO_TEXTURE;
    sphere.material.color = gs_v4(0.8, 0.3, 0.1, 1.0);

    plane.mesh = mesh_plane();
    plane.transform = gs_vqs_default();
    plane.transform.scale = gs_v3s(30.f);
    plane.material.texture = NO_TEXTURE;
    plane.material.color = gs_v4(0.3, 0.8, 0.1, 1.0);

    gs_dyn_array(card_data_t) hand_data = hand_get_random(true, true, true);
    hand = NULL;
    for (int i = 0; i < gs_dyn_array_size(hand_data); i++) {
        card_entity_t c = card_entity_create(hand_data[i]);
        card_entity_bake_texture(&engine.gsi, c);
        gs_dyn_array_push(hand, c);
    }

    card_entities_position_as_hand(&engine.camera, hand, false);

    card = card_entity_create(card_get_random(true, false, false));
    card_entity_bake_texture(&engine.gsi, card);
}

void update() {
    // if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
    //     state.mode = MENU;
    // }
    //
    // gs_gui_begin(&engine.gui, NULL);
    // card_library_gui(&engine);
    // if (state.mode == MENU) {
    //     state.mode = gui_show_menu(&state);
    //     if (state.mode == LIBRARY) {
    //         card_library_init();
    //     }
    //     if (state.mode == CARD_GAME) {
    //         card_game = (card_game_state_t){0};
    //         card_game.simulate_player = true;
    //         card_game.game_speed = 500.0f;
    //         card_game.simulation_count = 5000;
    //         card_game.simulate_color_v_color = false;
    //         for (int i = 0; i < 100; i++) card_game.winning_card_counts[i] = 0;
    //
    //         gs_dyn_array(card_state_t) player_hand = card_game.simulate_color_v_color ? hand_get_random_color() : hand_get_random(true, true, true);
    //         gs_dyn_array(card_state_t) opponent_hand = card_game.simulate_color_v_color ? hand_get_random_color() : hand_get_random(true, true, true);
    //         card_game_init(&card_game, &state, player_hand, opponent_hand);
    //     }
    // } else if (state.mode == LIBRARY) {
    //     state.mode = card_library_gui(&state);
    //     if (state.mode == CARD_GAME) {
    //         card_game = (card_game_state_t){0};
    //         card_game.game_speed = 1.0f;
    //         gs_dyn_array(card_state_t) opponent_hand = hand_get_random(card_library_red_enabled, card_library_green_enabled, card_library_blue_enabled);
    //         card_game_init(&card_game, &state, card_library_hand, opponent_hand);
    //     }
    // } else if (state.mode == CARD_GAME && card_game.simulate_player) {
    //     card_game_show_simulation_gui(&card_game, &state);
    // }
    // gs_gui_end(&engine.gui);

   // game_render_begin(&state);
    // if (state.mode == LIBRARY) {
    //     card_library_update(&state);
    // }
    // if (state.mode == CARD_GAME) {
    //     card_game_update(&card_game, &state);
    // }

    // gs_gui_render(&state.gui_ctx, &state.command_buffer);
   // game_render_end(&state);

    gs_gui_begin(&engine.gui, NULL);
    card_library_gui(&engine);
    gs_gui_end(&engine.gui);

    float dt = gs_platform_delta_time();

    // WASD movement
    gs_vec3 move = gs_v3(0.f, 0.f, 0.f);
    if (gs_platform_key_down(GS_KEYCODE_W)) move.z -= 1.f;
    if (gs_platform_key_down(GS_KEYCODE_S)) move.z += 1.f;
    if (gs_platform_key_down(GS_KEYCODE_A)) move.x -= 1.f;
    if (gs_platform_key_down(GS_KEYCODE_D)) move.x += 1.f;

    if (gs_vec3_len(move) > 0.f)
    {
        move = gs_vec3_norm(move);
        move = gs_vec3_scale(move, player_speed * dt);
        player_pos = gs_vec3_add(player_pos, move);
    }

    engine.camera.transform.position = gs_vec3_add(player_pos, camera_offset);

    gs_vec2 fbs = gs_platform_framebuffer_sizev(gs_platform_main_window());
    gs_mat4 vp = gs_camera_get_view_projection(&engine.camera, (uint32_t)fbs.x, (uint32_t)fbs.y);

    gs_graphics_clear_action_t clear_action = gs_default_val();
    clear_action.flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH;
    clear_action.color[0] = 0.08f;
    clear_action.color[1] = 0.08f;
    clear_action.color[2] = 0.10f;
    clear_action.color[3] = 1.f;
    gs_graphics_clear_desc_t clear = gs_default_val();
    clear.actions = &clear_action;
    clear.size = sizeof(clear_action);

    gs_graphics_renderpass_begin(&engine.cb, (gs_handle(gs_graphics_renderpass_t)){0});
    gs_graphics_clear(&engine.cb, &clear);
    gs_graphics_set_viewport(&engine.cb, 0, 0, (uint32_t)fbs.x, (uint32_t)fbs.y);
    gs_graphics_pipeline_bind(&engine.cb, engine.standard_shader.pipeline);

    draw_entity(&plane, vp, &engine.standard_shader, &engine);
    sphere.transform.position = player_pos;
    draw_entity(&sphere, vp, &engine.standard_shader, &engine);

    card_library_update(&engine);

    gs_gui_render(&engine.gui, &engine.cb);
    gs_graphics_renderpass_end(&engine.cb);
    gs_graphics_command_buffer_submit(&engine.cb);
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
