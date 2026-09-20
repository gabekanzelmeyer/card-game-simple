#define GS_IMPL
#define GS_IMMEDIATE_DRAW_IMPL
#define GS_GUI_IMPL

#include "gs.h"

#include "engine.h"
#include "engine_shapes.h"
#include "card_entity.h"
#include "card_database.h"
#include "card_library.h"
#include "card_game.h"

static card_game_state_t card_game = {0};

gs_vec3 player_pos;
float player_speed = 6.f;
gs_vec3 camera_offset;
static engine_t engine;
entity_t sphere;
entity_t plane;
entity_t cube;
entity_t capsule;
enum game_mode mode;


void init() {
    srand(time(NULL));
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
    sphere.transform.position = gs_v3(0, 0.5, 0);
    sphere.prev = sphere.transform;
    sphere.next = sphere.transform;
    sphere.material.texture = NO_TEXTURE;
    sphere.material.color = gs_v4(0.8, 0.3, 0.1, 1.0);

    plane.mesh = mesh_plane();
    plane.transform = gs_vqs_default();
    plane.transform.scale = gs_v3s(30.f);
    plane.material.texture = NO_TEXTURE;
    plane.material.color = gs_v4(0.4, 0.4, 0.4, 1.0);

    cube.mesh = mesh_cube();
    cube.transform = gs_vqs_default();
    cube.transform.position = gs_v3(4, 0.5, 4);
    cube.material.texture = NO_TEXTURE;
    cube.material.color = gs_v4(0.1, 0.3, 0.8, 1.0);

    capsule.mesh = mesh_capsule(0.5f, 1.0f, 32, 8);
    capsule.transform = gs_vqs_default();
    capsule.transform.position = gs_v3(-4, 1, -4);
    capsule.material.texture = NO_TEXTURE;
    capsule.material.color = gs_v4(0.1, 0.8, 0.3, 1.0);

    mode = WORLD;
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        mode = WORLD;
    }

    gs_gui_begin(&engine.gui, NULL);
    if (mode == LIBRARY) {
        mode = card_library_gui(&engine);

        if (mode == CARD_GAME) {
            card_game = (card_game_state_t){0};
            card_game.game_speed = 1.0f;
            gs_dyn_array(card_data_t) player_hand = NULL;
            for (int i = 0; i < gs_dyn_array_size(card_library_hand); i++) {
                gs_dyn_array_push(player_hand, card_library_hand[i].data);
            }
            gs_dyn_array(card_data_t) opponent_hand = hand_get_random(card_library_red_enabled, card_library_green_enabled, card_library_blue_enabled);
            card_game_init(&engine, &card_game, player_hand, opponent_hand);
        } else if (mode == SIM_CARD_GAME) {
            card_game = (card_game_state_t){0};
            card_game.simulate_player = true;
            card_game.game_speed = 500.0f;
            card_game.simulation_count = 5000;
            card_game.simulate_color_v_color = false;
            for (int i = 0; i < 100; i++) card_game.winning_card_counts[i] = 0;

            gs_dyn_array(card_data_t) player_hand = card_game.simulate_color_v_color ? hand_get_random_color() : hand_get_random(true, true, true);
            gs_dyn_array(card_data_t) opponent_hand = card_game.simulate_color_v_color ? hand_get_random_color() : hand_get_random(true, true, true);
            card_game_init(&engine, &card_game, player_hand, opponent_hand);
        }
    } else if (mode == CARD_GAME && card_game.simulation_count > 0) {
        card_game_show_simulation_gui(&engine, &card_game);
    }
    gs_gui_end(&engine.gui);

    float dt = gs_platform_delta_time();

    // WASD movement
    gs_vec3 move = gs_v3(0.f, 0.f, 0.f);
    if (mode == WORLD) {
        if (gs_platform_key_down(GS_KEYCODE_W)) move.z -= 1.f;
        if (gs_platform_key_down(GS_KEYCODE_S)) move.z += 1.f;
        if (gs_platform_key_down(GS_KEYCODE_A)) move.x -= 1.f;
        if (gs_platform_key_down(GS_KEYCODE_D)) move.x += 1.f;
    }
    if (gs_platform_key_pressed(GS_KEYCODE_SPACE)) {
        if (mode == WORLD) {
            mode = LIBRARY;
            card_library_init();
        } else if (mode == LIBRARY) {
            mode = WORLD;
        }
    }

    if (gs_vec3_len(gs_vec3_sub(sphere.next.position, sphere.prev.position)) > 0)  {
        sphere.lerp += dt / 0.2f;
    }
    if (gs_vec3_len(move) > 0.f && sphere.lerp >= 1.0f) {
        sphere.prev = sphere.next;
        sphere.next.position = gs_vec3_add(sphere.next.position, move);
        sphere.lerp -= 1.0;
    } else if (gs_vec3_len(move) > 0.f && sphere.lerp == 0.0f) {
        sphere.prev = sphere.transform;
        sphere.next.position = gs_vec3_add(sphere.next.position, move);
        sphere.lerp = 0;
    } else if (sphere.lerp >= 1.0f) {
        sphere.prev = sphere.next;
        sphere.lerp = 0;
    }
    lerp_transform_linear(&sphere.transform, &sphere.prev, &sphere.next, sphere.lerp);


    engine.camera.transform.position = gs_vec3_add(sphere.transform.position, camera_offset);

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
    draw_entity(&sphere, vp, &engine.standard_shader, &engine);
    draw_entity(&cube, vp, &engine.standard_shader, &engine);
    draw_entity(&capsule, vp, &engine.standard_shader, &engine);

    if (mode == LIBRARY) {
        card_library_update(&engine);
    } else if (mode == CARD_GAME || mode == SIM_CARD_GAME) {
        mode = card_game_update(&engine, &card_game);
    }

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
