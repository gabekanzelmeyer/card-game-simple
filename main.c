#define GS_IMPL
#include "gs.h"

#define GS_IMMEDIATE_DRAW_IMPL
#include "util/gs_idraw.h"

#include "card.h"

typedef struct {
    gs_command_buffer_t command_buffer;
    gs_immediate_draw_t immediate_draw;
    gs_camera_t camera;
} game_state;

static game_state state = {0};

static card_state card = {0};

void init() {
    state.command_buffer = gs_command_buffer_new();
    state.immediate_draw = gs_immediate_draw_new();
    state.camera = gs_camera_perspective();
    state.camera .transform.position = gs_v3(0.f, 0.f, 3.f);

    card_global_init();
    card = card_new("Test Card", 2, 2);

    card_bake_texture(&card, &state.immediate_draw);
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        gs_quit();
    }

    float dt = gs_platform_delta_time();

    // translate card
    float speed = 2.f * dt;
    if (gs_platform_key_down(GS_KEYCODE_A)) card.transform.position.x -= speed;
    if (gs_platform_key_down(GS_KEYCODE_D)) card.transform.position.x += speed;
    if (gs_platform_key_down(GS_KEYCODE_S)) card.transform.position.y -= speed;
    if (gs_platform_key_down(GS_KEYCODE_W)) card.transform.position.y += speed;

    // rotate card
    float rot_speed = 90.f * dt; // degrees/sec
    if (gs_platform_key_down(GS_KEYCODE_Q)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(0, 1, 0));
        card.transform.rotation = gs_quat_mul(dq, card.transform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_E)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(0, 1, 0));
        card.transform.rotation = gs_quat_mul(dq, card.transform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_Z)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(1, 0, 0));
        card.transform.rotation = gs_quat_mul(dq, card.transform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_C)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(1, 0, 0));
        card.transform.rotation = gs_quat_mul(dq, card.transform.rotation);
    }

    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);

    gs_mat4 model = gs_vqs_to_mat4(&card.transform);
    gs_mat4 vp = gs_camera_get_view_projection(&state.camera, (int32_t)fbw, (int32_t)fbh);
    gs_mat4 mvp = gs_mat4_mul(vp, model);

    gs_graphics_clear_action_t clear = {.flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH, .color = {0.1f, 0.1f, 0.15f, 1.f}};
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(&state.command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT); // default (screen) render pass
    gs_graphics_clear(&state.command_buffer, &clear_desc);
    gs_graphics_set_viewport(&state.command_buffer, 0, 0, fbw, fbh);

    gs_graphics_pipeline_bind(&state.command_buffer, card_global.card_pipeline);

    gs_graphics_bind_vertex_buffer_desc_t vbuf = {.buffer = card_global.card_vertex_buffer};
    gs_graphics_bind_index_buffer_desc_t  ibuf = {.buffer = card_global.card_index_buffer};
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        {.uniform = card.uniform_mvp, .data = &mvp, .binding = 0},
        {.uniform = card.uniform_texture, .data = &card.texture, .binding = 0}
    };
    gs_graphics_bind_desc_t binds = {
        .vertex_buffers = {.desc = &vbuf},
        .index_buffers  = {.desc = &ibuf},
        .uniforms       = {.desc = uniforms, .size = sizeof(uniforms)}
    };
    gs_graphics_apply_bindings(&state.command_buffer, &binds);
    gs_graphics_draw(&state.command_buffer, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});
    gs_graphics_renderpass_end(&state.command_buffer);
    gs_graphics_command_buffer_submit(&state.command_buffer);
}

gs_app_desc_t gs_main(int32_t argc, char** argv)
{
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
