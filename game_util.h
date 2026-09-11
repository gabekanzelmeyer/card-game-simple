#ifndef GAME_UTIL_H
#define GAME_UTIL_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

// #define CGLTF_IMPLEMENTATION
// #include "external/cgltf/cgltf.h"

#include "card_renderer.h"

// macro that allows for erasing and element from a gs_dyn_array and keeping the order
// used to remove cards from player / opponent hands
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

#define HOVER_SCREEN_SIZE 300
#define HAND_SPACING 1.5f
#define HAND_FAN_ANGLE 2.0f
#define HAND_CURVE_AMOUNT 0.02f
#define HAND_Y_POSITION_OFFSET 2.6f

typedef struct vertex_t {
    gs_vec3 position;
    gs_vec3 normal;
    gs_vec2 uv;
} vertex_t;

typedef struct mesh_t {
    gs_handle(gs_graphics_vertex_buffer_t) vbo;
    gs_handle(gs_graphics_index_buffer_t) ibo;
    uint32_t index_count;
} mesh_t;


enum game_mode {
    MENU,
    LIBRARY,
    CARD_GAME,
    WORLD
};

typedef struct {
    gs_command_buffer_t command_buffer;
    gs_immediate_draw_t immediate_draw;
    gs_gui_context_t gui_ctx;
    gs_camera_t camera;
    enum game_mode mode;

    gs_asset_font_t ui_font;
    card_render_data_t *card_renderer;
} game_state_t;

void game_state_init(game_state_t *state, card_render_data_t *card_renderer) {

    gs_gui_init(&state->gui_ctx, gs_platform_main_window());

    state->command_buffer = gs_command_buffer_new();
    state->immediate_draw = gs_immediate_draw_new();
    state->camera = gs_camera_perspective();
    state->camera .transform.position = gs_v3(0.f, 0.f, 6.5f);


    if (!gs_asset_font_load_from_file("assets/font.otf", &state->ui_font, 120)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }

    gs_gui_style_element_t font_style[] = {{ .type = GS_GUI_STYLE_FONT, .font = &state->ui_font}};

    gs_gui_set_element_style(&state->gui_ctx, GS_GUI_ELEMENT_TEXT, GS_GUI_ELEMENT_STATE_DEFAULT, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state->gui_ctx, GS_GUI_ELEMENT_TEXT, GS_GUI_ELEMENT_STATE_HOVER, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state->gui_ctx, GS_GUI_ELEMENT_TEXT, GS_GUI_ELEMENT_STATE_FOCUS, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state->gui_ctx, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_DEFAULT, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state->gui_ctx, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_HOVER, font_style, sizeof(font_style));
    gs_gui_set_element_style(&state->gui_ctx, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_FOCUS, font_style, sizeof(font_style));

    state->mode = WORLD;
    state->card_renderer = card_renderer;
}

void game_render_begin(game_state_t *state) {
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_graphics_clear_action_t clear = {
        .flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH,
        .color = {0.1f, 0.1f, 0.15f, 1.f}
    };
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(&state->command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT); // default (screen) render pass
    gs_graphics_clear(&state->command_buffer, &clear_desc);
    gs_graphics_set_viewport(&state->command_buffer, 0, 0, fbw, fbh);
}

void game_render_end(game_state_t *state) {
    gs_graphics_renderpass_end(&state->command_buffer);
    gs_graphics_command_buffer_submit(&state->command_buffer);
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

    // calculate screen coords
    out_screen->x = ((ndc_x + 1.0f) * 0.5f) * screen_width;
    out_screen->y = ((1.0f - ndc_y) * 0.5f) * screen_height; // Inverted Y for standard 2D screen coordinate spaces

    return true;
}

void lerp_card_transform(card_state_t *card, float dt) {
    card->lerp += dt / card->anim_duration;
    if (card->lerp >= 1) {
        card->lerp = 1;
    }

    card->transform.position.x = gs_interp_smoothstep(card->prev_transform.position.x, card->target_transform.position.x, card->lerp);
    card->transform.position.y = gs_interp_smoothstep(card->prev_transform.position.y, card->target_transform.position.y, card->lerp);
    card->transform.position.z = gs_interp_smoothstep(card->prev_transform.position.z, card->target_transform.position.z, card->lerp);

    card->transform.scale.x = gs_interp_smoothstep(card->prev_transform.scale.x, card->target_transform.scale.x, card->lerp);
    card->transform.scale.y = gs_interp_smoothstep(card->prev_transform.scale.y, card->target_transform.scale.y, card->lerp);
    card->transform.scale.z = gs_interp_smoothstep(card->prev_transform.scale.z, card->target_transform.scale.z, card->lerp);

    card->transform.rotation.x = gs_interp_smoothstep(card->prev_transform.rotation.x, card->target_transform.rotation.x, card->lerp);
    card->transform.rotation.y = gs_interp_smoothstep(card->prev_transform.rotation.y, card->target_transform.rotation.y, card->lerp);
    card->transform.rotation.z = gs_interp_smoothstep(card->prev_transform.rotation.z, card->target_transform.rotation.z, card->lerp);
    card->transform.rotation.w = gs_interp_smoothstep(card->prev_transform.rotation.w, card->target_transform.rotation.w, card->lerp);
}

void set_card_animation(card_state_t *card, gs_vqs_t target_transform, float duration) {
    card->prev_transform.position = card->transform.position;
    card->prev_transform.rotation = card->transform.rotation;
    card->prev_transform.scale = card->transform.scale;
    card->target_transform.position = target_transform.position;
    card->target_transform.rotation = target_transform.rotation;
    card->target_transform.scale = target_transform.scale;
    card->anim_duration = duration;
    card->lerp = 0.0f;
}

void position_hand_cards(card_state_t *cards, bool bottom_of_screen) {
    float spacing, fan_angle, curve_amount, y_offset;
    spacing = HAND_SPACING;
    if (bottom_of_screen) {
        fan_angle = HAND_FAN_ANGLE;
        curve_amount = -HAND_CURVE_AMOUNT;
        y_offset = -HAND_Y_POSITION_OFFSET;
    } else {
        fan_angle = -HAND_FAN_ANGLE;
        curve_amount = HAND_CURVE_AMOUNT;
        y_offset = HAND_Y_POSITION_OFFSET;
    }

    int count = gs_dyn_array_size(cards);
    float start_x = -spacing * (float)count / 2.f + spacing / 2.f;
    float start_tilt = fan_angle * (float)count / 2.f - fan_angle / 2.f;
    for (int i = 0; i < count; i++) {
        gs_vqs_t target = gs_vqs_default();
        target.position.x = start_x + i * spacing;
        target.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * curve_amount * (1.f / spacing) + y_offset;
        target.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * fan_angle)), gs_v3(0, 0, 1));
        set_card_animation(&cards[i], target, 0.2f);
    }
}

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
            gs_gui_rect_t play_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport, 500, 200, 0, -150, GS_GUI_LAYOUT_ANCHOR_CENTER);
            gs_gui_layout_set_next(&state->gui_ctx, play_rect, 0);
            if (gs_gui_button(&state->gui_ctx, "Play")) {
                result = LIBRARY;
            }
            gs_gui_rect_t sim_rect = gs_gui_layout_anchor(&state->gui_ctx.viewport, 500, 200, 0, 150, GS_GUI_LAYOUT_ANCHOR_CENTER);
            gs_gui_layout_set_next(&state->gui_ctx, sim_rect, 0);
            if (gs_gui_button(&state->gui_ctx, "Sim")) {
                result = CARD_GAME;
            }
        }
        gs_gui_window_end(&state->gui_ctx);
        return result;
}

// static mesh_t load_gltf_mesh(const char* filename) {
//     mesh_t result = gs_default_val();
//
//     cgltf_options options = {0};
//     cgltf_data* data = NULL;
//
//     cgltf_result res =
//     cgltf_parse_file(&options, filename, &data);
//
//     if (res != cgltf_result_success)
//     {
//         printf("Failed to parse glTF: %s\n", filename);
//         return result;
//     }
//
//     res = cgltf_load_buffers(&options, data, filename);
//
//     if (res != cgltf_result_success)
//     {
//         printf("Failed to load glTF buffers: %s\n", filename);
//         cgltf_free(data);
//         return result;
//     }
//
//     if (data->meshes_count == 0)
//     {
//         printf("glTF contains no meshes: %s\n", filename);
//         cgltf_free(data);
//         return result;
//     }
//
//     cgltf_mesh* src_mesh = &data->meshes[0];
//     cgltf_primitive* primitive = &src_mesh->primitives[0];
//
//     cgltf_accessor* position_accessor = NULL;
//     cgltf_accessor* normal_accessor = NULL;
//     cgltf_accessor* uv_accessor = NULL;
//
//     for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
//     {
//         cgltf_attribute* attr = &primitive->attributes[i];
//
//         if (attr->type == cgltf_attribute_type_position)
//             position_accessor = attr->data;
//
//         else if (attr->type == cgltf_attribute_type_normal)
//             normal_accessor = attr->data;
//
//         else if (attr->type == cgltf_attribute_type_texcoord)
//             uv_accessor = attr->data;
//     }
//
//     if (!position_accessor)
//     {
//         printf("Mesh has no positions\n");
//         cgltf_free(data);
//         return result;
//     }
//
//     cgltf_size vertex_count = position_accessor->count;
//
//     gs_dyn_array(vertex_t) vertices = NULL;
//
//     for (cgltf_size i = 0; i < vertex_count; ++i)
//     {
//         vertex_t v = gs_default_val();
//
//         cgltf_accessor_read_float(
//             position_accessor,
//             i,
//             &v.position.x,
//             3
//         );
//
//         if (normal_accessor)
//         {
//             cgltf_accessor_read_float(
//                 normal_accessor,
//                 i,
//                 &v.normal.x,
//                 3
//             );
//         }
//
//         if (uv_accessor)
//         {
//             cgltf_accessor_read_float(
//                 uv_accessor,
//                 i,
//                 &v.uv.x,
//                 2
//             );
//         }
//
//         gs_dyn_array_push(vertices, v);
//     }
//     gs_dyn_array(uint32_t) indices = NULL;
//
//     if (primitive->indices)
//     {
//         cgltf_accessor* index_accessor = primitive->indices;
//
//         for (cgltf_size i = 0; i < index_accessor->count; ++i)
//         {
//             cgltf_uint index =
//             cgltf_accessor_read_index(index_accessor, i);
//
//             gs_dyn_array_push(indices, (uint32_t)index);
//         }
//     }
//     gs_graphics_vertex_buffer_desc_t vdesc = gs_default_val();
//
//     vdesc.data = vertices;
//     vdesc.size = gs_dyn_array_size(vertices) * sizeof(vertex_t);
//     vdesc.usage = GS_GRAPHICS_BUFFER_USAGE_STATIC;
//
//     result.vbo =
//     gs_graphics_vertex_buffer_create(&vdesc);
//
//
//     gs_graphics_index_buffer_desc_t idesc = gs_default_val();
//
//     idesc.data = indices;
//     idesc.size =
//     gs_dyn_array_size(indices) * sizeof(uint32_t);
//     idesc.usage = GS_GRAPHICS_BUFFER_USAGE_STATIC;
//
//     result.ibo =
//     gs_graphics_index_buffer_create(&idesc);
//
//     result.index_count =
//     (uint32_t)gs_dyn_array_size(indices);
//
//     gs_dyn_array_free(vertices);
//     gs_dyn_array_free(indices);
//
//     cgltf_free(data);
//
//     return result;
// }


#endif
