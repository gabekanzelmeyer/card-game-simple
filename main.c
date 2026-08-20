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

#define NUM_CARDS 6
static card_state cards[NUM_CARDS];

void position_hand(card_state *cards, int count, float spacing, float tilt) {
    float start_x = -spacing * (float)count / 2.f + spacing / 2.f;
    float start_tilt = tilt * (float)count / 2.f - tilt / 2.f;
    for (int i = 0; i < count; i++) {
        cards[i].transform.position.x = start_x + i * spacing;
        cards[i].transform.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * -0.1f;
        cards[i].transform.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * tilt)), gs_v3(0, 0, 1));
    }
}

void init() {
    state.command_buffer = gs_command_buffer_new();
    state.immediate_draw = gs_immediate_draw_new();
    state.camera = gs_camera_perspective();
    state.camera .transform.position = gs_v3(0.f, 0.f, 5.f);

    card_global_init();

    cards[0] = card_new("Card 1", 6, 2, 0);
    cards[1] = card_new("Card 2", 1, 8, 1);
    cards[2] = card_new("Card 3", 3, 3, 2);
    cards[3] = card_new("Card 4", 1, 8, 3);
    cards[4] = card_new("Card 5", 3, 3, 4);
    cards[5] = card_new("Card 6", 1, 8, 5);

    position_hand(cards, NUM_CARDS, 1.f, 10.0f);

    // Bake each unique face into its atlas slot ONCE.
    // If you later add duplicate cards (same name/art), reuse an existing
    // slot instead of baking again.
    for (int i = 0; i < NUM_CARDS; ++i) {
        card_bake_texture(&cards[i], &state.immediate_draw);
    }
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        gs_quit();
    }

    float dt = gs_platform_delta_time();

    // translate card
    float speed = 2.f * dt;
    if (gs_platform_key_down(GS_KEYCODE_A)) cards[0].transform.position.x -= speed;
    if (gs_platform_key_down(GS_KEYCODE_D)) cards[0].transform.position.x += speed;
    if (gs_platform_key_down(GS_KEYCODE_S)) cards[0].transform.position.y -= speed;
    if (gs_platform_key_down(GS_KEYCODE_W)) cards[0].transform.position.y += speed;

    // rotate card
    float rot_speed = 90.f * dt; // degrees/sec
    if (gs_platform_key_down(GS_KEYCODE_Q)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(0, 1, 0));
        cards[0].transform.rotation = gs_quat_mul(dq, cards[0].transform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_E)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(0, 1, 0));
        cards[0].transform.rotation = gs_quat_mul(dq, cards[0].transform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_Z)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(1, 0, 0));
        cards[0].transform.rotation = gs_quat_mul(dq, cards[0].transform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_C)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(1, 0, 0));
        cards[0].transform.rotation = gs_quat_mul(dq, cards[0].transform.rotation);
    }


    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_projection = gs_camera_get_view_projection(&state.camera, (int32_t)fbw, (int32_t)fbh);

    gs_graphics_clear_action_t clear = {
        .flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH,
        .color = {0.1f, 0.1f, 0.15f, 1.f}
    };
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(&state.command_buffer, GS_GRAPHICS_RENDER_PASS_DEFAULT); // default (screen) render pass
    gs_graphics_clear(&state.command_buffer, &clear_desc);
    gs_graphics_set_viewport(&state.command_buffer, 0, 0, fbw, fbh);

    card_render_instanced(cards, NUM_CARDS, &state.command_buffer, view_projection);
    //card_render_single(&card, &state.command_buffer, view_projection);

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
