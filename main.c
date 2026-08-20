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
        cards[i].transform.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * -0.08 * (1.f / spacing);
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

    position_hand(cards, NUM_CARDS, 0.9f, 10.0f);

    // Bake each unique face into its atlas slot ONCE.
    // If you later add duplicate cards (same name/art), reuse an existing
    // slot instead of baking again.
    for (int i = 0; i < NUM_CARDS; ++i) {
        card_bake_texture(&cards[i], &state.immediate_draw);
    }
}

bool world_to_screen(gs_vec3 world_pos, gs_vec2* out_screen, gs_mat4 view_proj, float screen_width, float screen_height) {
    // 1. Multiply 3D point by View-Projection Matrix to get Clip Space
    float x = world_pos.x * view_proj.m[0][0] + world_pos.y * view_proj.m[1][0] + world_pos.z * view_proj.m[2][0] + view_proj.m[3][0];
    float y = world_pos.x * view_proj.m[0][1] + world_pos.y * view_proj.m[1][1] + world_pos.z * view_proj.m[2][1] + view_proj.m[3][1];
    float z = world_pos.x * view_proj.m[0][2] + world_pos.y * view_proj.m[1][2] + world_pos.z * view_proj.m[2][2] + view_proj.m[3][2];
    float w = world_pos.x * view_proj.m[0][3] + world_pos.y * view_proj.m[1][3] + world_pos.z * view_proj.m[2][3] + view_proj.m[3][3];

    // 2. Object is behind the camera if W is less than or equal to 0
    if (w <= 0.0f) {
        return false;
    }

    // 3. Perspective Divide to get Normalized Device Coordinates (NDC) [-1, 1]
    float ndc_x = x / w;
    float ndc_y = y / w;

    // 4. Convert NDC into actual pixel screen positions
    out_screen->x = ((ndc_x + 1.0f) * 0.5f) * screen_width;
    out_screen->y = ((1.0f - ndc_y) * 0.5f) * screen_height; // Inverted Y for standard 2D screen coordinate spaces

    return true;
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        gs_quit();
    }

    float dt = gs_platform_delta_time();
    gs_vec2 mouse_pos = gs_platform_mouse_positionv();

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

    // NOTE: cards are drawn depth wise in order, so sort based on draw order before rendering
    // In that same spirit, we should iterate backwards over the cards to check if any are hovered
    for (int i = NUM_CARDS - 1; i >= 0; i--) {
        cards[i].transform.scale = gs_v3(1.f, 1.f, 1.f);
    }
    for (int i = NUM_CARDS - 1; i >= 0; i--) {
        gs_vec2 screen_pos;
        world_to_screen(cards[i].transform.position, &screen_pos, view_projection, fbw, fbh);
        if (gs_vec2_len(gs_vec2_sub(mouse_pos, screen_pos)) < 300)  {
            cards[i].transform.scale = gs_v3(1.2f, 1.2f, 1.2f);
            break;
        }
    }
    card_render_instanced(cards, NUM_CARDS, &state.command_buffer, view_projection);

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
