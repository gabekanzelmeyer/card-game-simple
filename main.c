#include <stdio.h>

#define GS_IMPL
#include "gs.h"

typedef struct {
    gs_command_buffer_t cb;
    gs_handle(gs_graphics_pipeline_t) pip;
    gs_handle(gs_graphics_vertex_buffer_t) vbo;
    gs_handle(gs_graphics_index_buffer_t) ibo;
    gs_handle(gs_graphics_shader_t) shader;
    gs_handle(gs_graphics_uniform_t) u_mvp;
    gs_handle(gs_graphics_uniform_t) u_tex;
    gs_handle(gs_graphics_texture_t) tex;

    gs_camera_t cam;
    gs_vqs card_xform;

} game_state;

typedef struct { float x, y, z, u, v; } vert_t;

static game_state state = {0};

static const char* v_src =
"#version 330 core\n"
"layout(location = 0) in vec3 a_pos;\n"
"layout(location = 1) in vec2 a_uv;\n"
"uniform mat4 u_mvp;\n"
"out vec2 uv;\n"
"void main() {\n"
"   uv = a_uv;\n"
"   gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
"}\n";

static const char* f_src =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 frag_color;\n"
"uniform sampler2D u_tex;\n"
"void main() {\n"
"   frag_color = texture(u_tex, uv);\n"
"}\n";


void init() {
    state.cb = gs_command_buffer_new();

    vert_t verts[] = {
        {-0.5f, -0.5f, 0.f,  0.f, 0.f},
        { 0.5f, -0.5f, 0.f,  1.f, 0.f},
        { 0.5f,  0.5f, 0.f,  1.f, 1.f},
        {-0.5f,  0.5f, 0.f,  0.f, 1.f},
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

    state.vbo = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t){
            .data = verts,
            .size = sizeof(verts)
        }
    );

    state.ibo = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t){
            .data = indices,
            .size = sizeof(indices)
        }
    );

    gs_graphics_shader_source_desc_t sources[] = {
        {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX,   .source = v_src},
        {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = f_src}
    };
    state.shader = gs_graphics_shader_create(&(gs_graphics_shader_desc_t) {
            .sources = sources,
            .size = sizeof(sources),
            .name = "card_shader"
        }
    );


    gs_graphics_vertex_attribute_desc_t layout[] = {
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv"}
    };
    state.pip = gs_graphics_pipeline_create(&(gs_graphics_pipeline_desc_t) {
            .raster = {
                .shader = state.shader,
                .primitive = GS_GRAPHICS_PRIMITIVE_TRIANGLES,
                .index_buffer_element_size = sizeof(uint16_t)
            },
            .layout = {
                .attrs = layout,
                .size = sizeof(layout)
            },
            .depth = {
                .func = GS_GRAPHICS_DEPTH_FUNC_LESS
            }
        }
    );

    gs_graphics_uniform_layout_desc_t mvp_layout = {.type = GS_GRAPHICS_UNIFORM_MAT4};
    state.u_mvp = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_mvp",
            .stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
            .layout = &mvp_layout,
            .layout_size = sizeof(mvp_layout)
        }
    );

    gs_graphics_uniform_layout_desc_t tex_layout = {.type = GS_GRAPHICS_UNIFORM_SAMPLER2D};
    state.u_tex = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_tex",
            .stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
            .layout = &tex_layout,
            .layout_size = sizeof(tex_layout)
        }
    );

    gs_graphics_texture_desc_t tex_desc = gs_default_val();
    int32_t tex_w = 0, tex_h = 0;
    uint32_t num_comps = 0;
    void* tex_data = NULL;
    bool32_t ok = gs_util_load_texture_data_from_file(
        "card.png",
        &tex_w, &tex_h,
        &num_comps,
        &tex_data,
        true           // flip_vertically_on_load
    );
    if (ok) {
        tex_desc.width = (uint32_t)tex_w;
        tex_desc.height = (uint32_t)tex_h;
        tex_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
        tex_desc.data[0] = tex_data;
        tex_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
        tex_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
        state.tex = gs_graphics_texture_create(&tex_desc);
        printf("w: %i h: %i\n", tex_desc.width, tex_desc.height);
        gs_free(tex_data);
    } else {
        gs_println("WARNING: failed to load card.png");
    }

    state.cam = gs_camera_perspective();
    state.cam.transform.position = gs_v3(0.f, 0.f, 3.f);
    state.card_xform = gs_vqs_default();
}

void update() {
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        gs_quit();
    }

    float dt = gs_platform_delta_time();

    // ---- Move the card with WASD (X/Y plane) ----
    float speed = 2.f * dt;
    if (gs_platform_key_down(GS_KEYCODE_A)) state.card_xform.position.x -= speed;
    if (gs_platform_key_down(GS_KEYCODE_D)) state.card_xform.position.x += speed;
    if (gs_platform_key_down(GS_KEYCODE_S)) state.card_xform.position.y -= speed;
    if (gs_platform_key_down(GS_KEYCODE_W)) state.card_xform.position.y += speed;

    // ---- Rotate the card with Q/E (around Y) and Z/C (around X) ----
    float rot_speed = 90.f * dt; // degrees/sec
    if (gs_platform_key_down(GS_KEYCODE_Q)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(0, 1, 0));
        state.card_xform.rotation = gs_quat_mul(dq, state.card_xform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_E)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(0, 1, 0));
        state.card_xform.rotation = gs_quat_mul(dq, state.card_xform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_Z)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(1, 0, 0));
        state.card_xform.rotation = gs_quat_mul(dq, state.card_xform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_C)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(1, 0, 0));
        state.card_xform.rotation = gs_quat_mul(dq, state.card_xform.rotation);
    }

    // ---- Build MVP ----
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);

    gs_mat4 model = gs_vqs_to_mat4(&state.card_xform);
    gs_mat4 vp    = gs_camera_get_view_projection(&state.cam, (int32_t)fbw, (int32_t)fbh);
    gs_mat4 mvp   = gs_mat4_mul(vp, model);

    // ---- Render ----
    gs_command_buffer_t* cb = &state.cb;

    gs_graphics_clear_action_t clear = {.flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH, .color = {0.1f, 0.1f, 0.15f, 1.f}};
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(cb, GS_GRAPHICS_RENDER_PASS_DEFAULT); // default (screen) render pass
    gs_graphics_clear(cb, &clear_desc);
    gs_graphics_set_viewport(cb, 0, 0, fbw, fbh);

    gs_graphics_pipeline_bind(cb, state.pip);

    gs_graphics_bind_vertex_buffer_desc_t vbuf = {.buffer = state.vbo};
    gs_graphics_bind_index_buffer_desc_t  ibuf = {.buffer = state.ibo};
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        {.uniform = state.u_mvp, .data = &mvp,        .binding = 0},
        {.uniform = state.u_tex, .data = &state.tex,  .binding = 0}
    };

    gs_graphics_bind_desc_t binds = {
        .vertex_buffers = {.desc = &vbuf},
        .index_buffers  = {.desc = &ibuf},
        .uniforms       = {.desc = uniforms, .size = sizeof(uniforms)}
    };
    gs_graphics_apply_bindings(cb, &binds);

    gs_graphics_draw(cb, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});

    gs_graphics_renderpass_end(cb);
    gs_graphics_command_buffer_submit(cb);

}

void shutdown() {
}

gs_app_desc_t gs_main(int32_t argc, char** argv){
    return (gs_app_desc_t) {
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .window = {
            .title = "Card Game",
            .width = 1600,
            .height = 1000
        }
    };
}
