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
// #include "game_util.h"

// static game_state_t state = {0};
// static card_game_state_t card_game = {0};
// static card_render_data_t card_renderer = {0};

gs_vec3 player_pos;
float player_speed = 6.f;
gs_vec3 camera_offset;
static engine_t engine;
static shader_t standard_shader;
static gs_asset_font_t font;
static gs_handle(gs_graphics_texture_t) non_texture;
static gs_handle(gs_graphics_texture_t) test_texture;
entity_t sphere;
entity_t plane;
entity_t card;


void init() {
    srand(time(NULL));
    // card_renderer_init(&card_renderer);
    // card_database_init();
    // game_state_init(&state, &card_renderer);

    camera_offset = gs_v3(0.f, 18.f, 10.f);
    player_pos = gs_v3(0.f, 0.5f, 0.f);

    engine = (engine_t){0};
    engine.cb = gs_command_buffer_new();
    engine.gsi = gs_immediate_draw_new();

    engine.camera = gs_camera_perspective();
    // engine.camera = gs_camera_default();
    // engine.camera.proj_type = GS_PROJECTION_TYPE_ORTHOGRAPHIC;
    // engine.camera.ortho_scale = 10.f;
    engine.camera.transform.position = gs_vec3_add(player_pos, camera_offset);
    engine.camera.transform.rotation = gs_quat_angle_axis(gs_deg2rad(-60.f), gs_v3(1.f, 0.f, 0.f));
    engine.camera.transform.scale = gs_v3s(1.f);
    engine.camera.fov = 60.f;
    engine.camera.near_plane = 0.1f;
    engine.camera.far_plane = 1000.f;

    standard_shader = shader_standard();

    uint8_t white[4] = {255, 255, 255, 255};
    gs_graphics_texture_desc_t non_texture_desc = gs_default_val();
    non_texture_desc.width = 1;
    non_texture_desc.height = 1;
    non_texture_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
    non_texture_desc.data[0] = white;
    non_texture_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    non_texture_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    non_texture = gs_graphics_texture_create(&non_texture_desc);

    sphere.mesh = mesh_sphere(0.5f, 16, 24);
    sphere.transform = gs_vqs_default();
    sphere.material.texture = non_texture;
    sphere.material.color = gs_v4(0.8, 0.3, 0.1, 1.0);

    plane.mesh = mesh_plane();
    plane.transform = gs_vqs_default();
    plane.transform.scale = gs_v3s(30.f);
    plane.material.texture = non_texture;
    plane.material.color = gs_v4(0.3, 0.8, 0.1, 1.0);

    int32_t tex_w = 0, tex_h = 0;
    uint32_t num_comps = 0;
    void* tex_data = NULL;
    bool32_t ok = gs_util_load_texture_data_from_file(
        "assets/card.png",
        &tex_w, &tex_h,
        &num_comps,
        &tex_data,
        true // flip_vertically_on_load
    );

    gs_graphics_texture_desc_t base_texture_desc = gs_default_val();
    base_texture_desc.width = (uint32_t)tex_w;
    base_texture_desc.height = (uint32_t)tex_h;
    base_texture_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
    base_texture_desc.data[0] = tex_data;
    base_texture_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    base_texture_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    test_texture = gs_graphics_texture_create(&base_texture_desc);
    // plane.material.color = gs_v4(1, 1, 1, 1);
    gs_free(tex_data);

    if (!gs_asset_font_load_from_file("assets/font.otf", &font, 100)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }
    render_texture_t render_texture = render_texture_create((uint32_t)tex_w, (uint32_t)tex_h);
    gs_handle(gs_graphics_framebuffer_t) frame_buffer = gs_graphics_framebuffer_create(&(gs_graphics_framebuffer_desc_t){0});
    gs_handle(gs_graphics_renderpass_t) render_pass = gs_graphics_renderpass_create(
        &(gs_graphics_renderpass_desc_t){
            .fbo = frame_buffer,
            .color = &render_texture.texture,
            .color_size = sizeof(render_texture.texture)
        }
    );

    render_text_on_texture(&engine.gsi,
                           test_texture,
                           render_pass,
                           (uint32_t)tex_w,
                           (uint32_t)tex_h,
                           "TEST",
                           &font,
                           255, 0, 0, 255);

    card.mesh = mesh_plane();
    card.transform = gs_vqs_default();
    card.transform.scale = gs_v3(1.f, 1.f, 1.7f);
    card.transform.position = gs_v3(0, 0.1, 0);
    card.material.texture = render_texture.texture;
    card.material.color = gs_v4s(1);
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

    // gs_gui_render(&state.gui_ctx, &state.command_buffer);
   // game_render_end(&state);

    float dt = gs_platform_delta_time();

    // ---- WASD movement ----
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
    gs_graphics_pipeline_bind(&engine.cb, standard_shader.pipeline);


    draw_entity(&plane, vp, &standard_shader, &engine);
    sphere.transform.position = player_pos;
    draw_entity(&sphere, vp, &standard_shader, &engine);


    gs_vec3 forward = gs_mat4_mul_vec3(
        gs_quat_to_mat4(engine.camera.transform.rotation),
        gs_v3(0.f, 0.f, -1.f)
    );

    gs_vec3 right = gs_mat4_mul_vec3(
        gs_quat_to_mat4(engine.camera.transform.rotation),
        gs_v3(1.f, 0.f, 0.f)
    );

    gs_vec3 up = gs_mat4_mul_vec3(
        gs_quat_to_mat4(engine.camera.transform.rotation),
        gs_v3(0.f, 1.f, 0.f)
    );
    card.transform.position =
    gs_vec3_add(
        engine.camera.transform.position,
        gs_vec3_add(
            gs_vec3_scale(forward, 5.f),
                    gs_vec3_add(
                        gs_vec3_scale(right, 3.f),
                                gs_vec3_scale(up, -2.f)
                    )
        )
    );
    card.transform.rotation = gs_quat_mul(
        engine.camera.transform.rotation,
        gs_quat_angle_axis(
            gs_deg2rad(90.f),
                           gs_v3(1.f, 0.f, 0.f)
        )
    );






    draw_entity(&card, vp, &standard_shader, &engine);

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
