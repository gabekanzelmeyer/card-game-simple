#define GS_IMPL
#define GS_IMMEDIATE_DRAW_IMPL
#define GS_GUI_IMPL

#include "gs.h"

#include "engine.h"
#include "engine_shapes.h"

// #include "card_renderer.h"
// #include "card_database.h"
// #include "card_library.h"
// #include "card_game.h"
// #include "world_renderer.h"
// #include "game_util.h"

// static game_state_t state = {0};
// static card_game_state_t card_game = {0};
// static card_render_data_t card_renderer = {0};

gs_vec3 player_pos;
gs_vec3 camera_offset;
static engine_t engine;
static shader_t standard_shader;
mesh_t sphere;
mesh_t plane;

void init() {
    srand(time(NULL));
    // card_renderer_init(&card_renderer);
    // card_database_init();
    // game_state_init(&state, &card_renderer);
    // world_init();

    camera_offset = gs_v3(0.f, 7.f, 6.f);
    player_pos = gs_v3(0.f, 0.5f, 0.f);

    engine = (engine_t){0};
    engine.cb = gs_command_buffer_new();
    // engine.camera = gs_camera_perspective();
    engine.camera = gs_camera_default();
    engine.camera.proj_type = GS_PROJECTION_TYPE_ORTHOGRAPHIC;
    engine.camera.ortho_scale = 10.f;
    engine.camera.transform.position = gs_vec3_add(player_pos, camera_offset);
    engine.camera.transform.rotation = gs_quat_angle_axis(gs_deg2rad(-30.f), gs_v3(1.f, 0.f, 0.f));
    engine.camera.transform.scale = gs_v3s(1.f);
    engine.camera.fov = 60.f;
    engine.camera.near_plane = 0.1f;
    engine.camera.far_plane = 1000.f;

    standard_shader = shader_standard();

    sphere = mesh_sphere(0.5f, 16, 24);
    plane = mesh_plane();
}

void update() {
    // if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
    //     state.mode = MENU;
    // }
    //
    // gs_gui_begin(&state.gui_ctx, NULL);
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
    // gs_gui_end(&state.gui_ctx);

   // game_render_begin(&state);
    // if (state.mode == LIBRARY) {
    //     card_library_update(&state);
    // }
    // if (state.mode == CARD_GAME) {
    //     card_game_update(&card_game, &state);
    // }

    // world_update();
    // gs_gui_render(&state.gui_ctx, &state.command_buffer);
   // game_render_end(&state);

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
    gs_graphics_pipeline_bind(&engine.cb, standard_shader.pipeline);

    gs_vqs ground_xform = gs_default_val();
    ground_xform.position = gs_v3(0.f, 0.f, 0.f);
    ground_xform.rotation = gs_quat_default();
    ground_xform.scale = gs_v3s(3.f);
    draw_mesh(&plane, gs_vqs_to_mat4(&ground_xform), vp, gs_v4(0.31f, 0.55f, 0.31f, 1.f), standard_shader, engine);

    gs_vqs sphere_xform = gs_default_val();
    sphere_xform.position = player_pos;
    sphere_xform.rotation = gs_quat_default();
    sphere_xform.scale = gs_v3s(1.f);
    draw_mesh(&sphere, gs_vqs_to_mat4(&sphere_xform), vp, gs_v4(0.86f, 0.24f, 0.24f, 1.f), standard_shader, engine);
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
