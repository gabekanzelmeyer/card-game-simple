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
#include "game.h"

static card_game_state_t card_game = {0};

gs_vec3 player_pos;
float player_speed = 6.f;
gs_vec3 camera_offset;
static engine_t engine;
static game_map_t map;
entity_t sphere;
entity_t cube;
entity_t capsule;
entity_t plane;
enum game_mode mode;


bool show_interaction = false;
game_interaction_t interaction = {0};

void init() {
    srand(time(NULL));
    camera_offset = gs_v3(0.f, 72.f, 40.f);
    player_pos = gs_v3(0.f, 0.5f, 0.f);

    engine_init(&engine);
    card_database_init();
    card_entites_init();
    card_library_init();

    engine.camera.transform.position = gs_vec3_add(player_pos, camera_offset);
    engine.camera.transform.rotation = gs_quat_angle_axis(gs_deg2rad(-60.f), gs_v3(1.f, 0.f, 0.f));

    sphere = entity_create();
    sphere.mesh = mesh_sphere(0.5f, 16, 24);
    sphere.material.color = gs_v4(0.8, 0.3, 0.1, 1.0);
    entity_t nose = entity_create();
    nose.mesh = mesh_sphere(0.25f, 16, 24);
    nose.material.color = gs_v4(0.3, 0.7, 0.7, 1.0);
    nose.transform.position.z = -0.5;
    gs_dyn_array_push(sphere.children, nose);

    cube = entity_create();
    cube.mesh = mesh_cube();
    cube.material.color = gs_v4(0.1, 0.3, 0.8, 1.0);

    capsule = entity_create();
    capsule.mesh = mesh_capsule(0.5f, 1.0f, 32, 8);
    capsule.material.color = gs_v4(0.1, 0.8, 0.3, 1.0);

    FILE *f = fopen("test.map", "r");
    if (f) {
        fclose(f);
        map = game_map_load("test.map");
        printf("map loaded\n");
    } else {
        int map_width = 50;
        int map_height = 50;
        float *heights = malloc(sizeof(float) * map_width * map_height);
        for (int j = 0; j < map_height; j++) {
            for (int i = 0; i < map_width; i++) {
                if (j > 5 && j < map_height - 5 && i > 5 && i < map_width - 5) {
                    heights[j * map_width + i] = 1.f;
                } else {
                    heights[j * map_width + i] = 0.f;
                }
            }
        }
        map = game_map_create(map_width, map_height, heights);
        free(heights);
        game_map_save(&map, "test.map");
    }

    game_map_add(&map, &sphere, 0, 0);
    game_map_add(&map, &cube, 4, 4);
    game_map_add(&map, &capsule, 8, 8);

    plane = entity_create();
    plane.mesh = mesh_grid_plane(map.width, map.height, map.height_map);
    plane.transform.position.x = map.width / 2 - 0.5;
    plane.transform.position.z = map.height / 2 - 0.5;
    plane.material.color = gs_v4(0.4, 0.4, 0.4, 1.0);

    mode = WORLD;

    interaction.text = "This is a test";
    interaction.transform = &capsule.transform;
    interaction.offset = gs_v2(0, -100);
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        mode = WORLD;
    }
    if (gs_platform_key_pressed(GS_KEYCODE_SPACE)) {
        if (mode == WORLD) {
            gs_vec3 local_forward = gs_v3(0.f, 0.f, -1.f);
            gs_vec3 world_forward = gs_quat_rotate(sphere.transform.rotation, local_forward);
            int in_front_x = sphere.next.position.x + lroundf(world_forward.x);
            int in_front_z = sphere.next.position.z + lroundf(world_forward.z);
            if (!game_map_is_tile_empty(&map, in_front_x, in_front_z)) {
                show_interaction = !show_interaction;
                interaction.transform = &map.tiles[in_front_z * map.width + in_front_x]->transform;
                if (!show_interaction) {
                    mode = LIBRARY;
                    card_library_init();
                }
            }
        } else if (mode == LIBRARY) {
            mode = WORLD;
        }
    }
    if (gs_platform_key_pressed(GS_KEYCODE_TAB)) {
        if (mode == WORLD) {
            mode = EDITOR;
        } else if (mode == EDITOR) {
            mode = WORLD;
        }
    }


    float dt = gs_platform_delta_time();
    gs_vec2 fbs = gs_platform_framebuffer_sizev(gs_platform_main_window());
    gs_mat4 vp = gs_camera_get_view_projection(&engine.camera, (uint32_t)fbs.x, (uint32_t)fbs.y);

    // WASD movement
    gs_vec3 move = gs_v3s(0);
    if (mode == WORLD) {
        if (gs_platform_key_down(GS_KEYCODE_W)) move.z -= 1.f;
        if (gs_platform_key_down(GS_KEYCODE_S)) move.z += 1.f;
        if (gs_platform_key_down(GS_KEYCODE_A)) move.x -= 1.f;
        if (gs_platform_key_down(GS_KEYCODE_D)) move.x += 1.f;
    }
    game_map_move(&map, &sphere, move, dt);

    engine.camera.transform.position = gs_vec3_add(sphere.transform.position, camera_offset);

    gs_gui_begin(&engine.gui, NULL);
    if (mode == WORLD) {
        if (gs_gui_window_begin_ex(&engine.gui, "main", gs_gui_rect(0, 0, 0, 0), NULL, NULL,
            GS_GUI_OPT_NOTITLE
            | GS_GUI_OPT_NORESIZE
            | GS_GUI_OPT_NOMOVE
            | GS_GUI_OPT_NOSCROLL
            | GS_GUI_OPT_NOCLOSE
            | GS_GUI_OPT_NOFRAME
            | GS_GUI_OPT_NOSTYLEBORDER
            | GS_GUI_OPT_NOSTYLESHADOW
            | GS_GUI_OPT_NOSTYLEBACKGROUND
            | GS_GUI_OPT_FULLSCREEN)) {

            if (show_interaction) {
                draw_hover_text(&engine, interaction.transform->position, interaction.offset, interaction.text);
            }

        }
        gs_gui_window_end(&engine.gui);
    } else if (mode == EDITOR) {
        if (gs_gui_window_begin_ex(&engine.gui, "main", gs_gui_rect(0, 0, 0, 0), NULL, NULL,
            GS_GUI_OPT_NOTITLE
            | GS_GUI_OPT_NORESIZE
            | GS_GUI_OPT_NOMOVE
            | GS_GUI_OPT_NOSCROLL
            | GS_GUI_OPT_NOCLOSE
            | GS_GUI_OPT_NOFRAME
            | GS_GUI_OPT_NOSTYLEBORDER
            | GS_GUI_OPT_NOSTYLESHADOW
            | GS_GUI_OPT_NOSTYLEBACKGROUND
            | GS_GUI_OPT_FULLSCREEN)) {
            gs_vec2 text_dimensions = gs_asset_font_text_dimensions(&engine.standard_font, "Editor", -1); // -1 means null-terminated string
            gs_gui_rect_t rect = gs_gui_layout_anchor(&engine.gui.viewport, text_dimensions.x, text_dimensions.y,
                                                      0, 30, GS_GUI_LAYOUT_ANCHOR_TOPCENTER);
            gs_gui_layout_set_next(&engine.gui, rect, 0);
            gs_gui_text(&engine.gui, "Editor");

            gs_vec3 mouse_terrain_coords;
            if (game_map_terrain_pick(&map, mouse_ray(&engine.camera), 100, &mouse_terrain_coords)) {
                gs_vec3 terrain_tile_pos = mouse_terrain_coords;
                terrain_tile_pos.x = lroundf(mouse_terrain_coords.x);
                terrain_tile_pos.z = lroundf(mouse_terrain_coords.z);

                for (int i = fmaxf(0, terrain_tile_pos.x - 1); i < fminf(map.width, terrain_tile_pos.x + 2); i++) {
                    for (int j = fmaxf(0, terrain_tile_pos.z - 1); j < fminf(map.height, terrain_tile_pos.z + 2); j++) {
                        char buf[4];
                        snprintf(buf, 4, "%.1f", map.height_map[j * map.width + i]);
                        draw_hover_text(&engine, gs_v3(i, map.height_map[j * map.width + i], j), gs_v2(0, 0), buf);
                    }
                }

                bool height_map_edited = false;
                if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON)) {
                    for (int i = fmaxf(0, terrain_tile_pos.x - 1); i < fminf(map.width, terrain_tile_pos.x + 2); i++) {
                        for (int j = fmaxf(0, terrain_tile_pos.z - 1); j < fminf(map.height, terrain_tile_pos.z + 2); j++) {
                            map.height_map[j * map.width + i] += 1;
                            height_map_edited = true;
                        }
                    }
                }
                if (gs_platform_mouse_pressed(GS_MOUSE_RBUTTON)) {
                    for (int i = fmaxf(0, terrain_tile_pos.x - 1); i < fminf(map.width, terrain_tile_pos.x + 2); i++) {
                        for (int j = fmaxf(0, terrain_tile_pos.z - 1); j < fminf(map.height, terrain_tile_pos.z + 2); j++) {
                            map.height_map[j * map.width + i] -= 1;
                            height_map_edited = true;
                        }
                    }
                }
                if (height_map_edited) {
                    gs_graphics_vertex_buffer_destroy(plane.mesh.vbo);
                    gs_graphics_index_buffer_destroy(plane.mesh.ibo);
                    plane.mesh = mesh_grid_plane(map.width, map.height, map.height_map);

                    game_map_save(&map, "test.map");
                }
            }
        }
        gs_gui_window_end(&engine.gui);
    } else if (mode == LIBRARY) {
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

    draw_entity(&engine, &plane, vp, &engine.standard_shader);
    draw_entity(&engine, &sphere, vp, &engine.standard_shader);
    draw_entity(&engine, &cube, vp, &engine.standard_shader);
    draw_entity(&engine, &capsule, vp, &engine.standard_shader);

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
