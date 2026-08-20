/*
    Gunslinger example: load a texture, render it on a quad in 3D,
    move it around, and rotate it, all viewed through a perspective camera.

    Verified against MrFrenik/gunslinger master (gs.h) directly.
    Build: single TU, #define GS_IMPL before including gs.h (done once, here).
*/
#define GS_IMPL
#include "gs.h"
#define GS_IMMEDIATE_DRAW_IMPL
#include "util/gs_idraw.h"   // Immediate-mode draw util: gives us text rendering (gsi_text) and a default font

typedef struct
{
    gs_command_buffer_t cb;
    gs_handle(gs_graphics_pipeline_t)      pip;
    gs_handle(gs_graphics_vertex_buffer_t) vbo;
    gs_handle(gs_graphics_index_buffer_t)  ibo;
    gs_handle(gs_graphics_shader_t)        shader;
    gs_handle(gs_graphics_uniform_t)       u_mvp;
    gs_handle(gs_graphics_uniform_t)       u_tex;

    // The raw card art loaded from disk (no text)
    gs_handle(gs_graphics_texture_t)       art_tex;

    // The "baked" card face: art + text composited together into one texture.
    // THIS is what actually gets mapped onto the 3D quad, which is why the
    // text rotates/moves perfectly in lockstep with the card -- it's just
    // pixels in the same texture as the artwork.
    gs_handle(gs_graphics_texture_t)       face_tex;
    gs_handle(gs_graphics_framebuffer_t)   face_fbo;
    gs_handle(gs_graphics_renderpass_t)    face_pass;
    uint32_t                               face_w, face_h;

    gs_immediate_draw_t gsi; // immediate-mode draw context, used to bake the face texture

    // Fonts baked at specific point sizes -- gunslinger bakes a font's glyphs
    // into a fixed-size bitmap atlas at load time, so "changing text size"
    // means loading (or re-loading) the font at a different point_size,
    // not scaling text after the fact.
    gs_asset_font_t font_title;
    gs_asset_font_t font_label;

    gs_camera_t cam;

    // The "card" quad's own transform in the world (VQS = pos/rot/scale)
    gs_vqs card_xform;
} app_t;

static app_t g_app = {0};

// Vertex: position(3) + uv(2)
typedef struct { float x, y, z, u, v; } vert_t;

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

void app_init();
void app_update();
void app_bake_card_face(bool32_t has_art, const char* title, const char* corner_label);

// Renders the base card art plus text into g_app.face_tex, ONCE (or whenever
// the card's text changes). This texture is what actually gets applied to the
// 3D quad, so the text is baked into the same UV space as the art -- it moves
// and rotates with the card automatically because it IS the card's surface.
void app_bake_card_face(bool32_t has_art, const char* title, const char* corner_label)
{
    gs_immediate_draw_t* gsi = &g_app.gsi;
    uint32_t w = g_app.face_w, h = g_app.face_h;

    // 2D orthographic projection matching the texture's pixel dimensions,
    // with (0,0) at the top-left -- standard for baking a UI-style texture.
    gsi_camera2D(gsi, w, h);

    // Draw the base card art stretched to fill the whole face texture.
    if (has_art) {
        gsi_texture(gsi, g_app.art_tex);
        gsi_rectvd(gsi, gs_v2(0.f, 0.f), gs_v2((float)w, (float)h),
                   gs_v2(0.f, 0.f), gs_v2(1.f, 1.f),
                   GS_COLOR_WHITE, GS_GRAPHICS_PRIMITIVE_TRIANGLES);
    }

    // Draw the card's title text near the top.
    gsi_text(gsi, 24.f, 24.f, title, &g_app.font_title, false, 20, 20, 20, 255);

    // Draw a small corner label (e.g. rank/value) near the top-left.
    gsi_text(gsi, 12.f, 60.f, corner_label, &g_app.font_label, false, 20, 20, 20, 255);

    // ---- Submit into the OFFSCREEN face_pass (not the default backbuffer) ----
    gs_command_buffer_t bake_cb = gs_command_buffer_new();
    gs_graphics_clear_action_t clear = {
        .flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH,
        .color = {0.f, 0.f, 0.f, 0.f} // fully transparent so any leftover alpha in the art shows through
    };
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};

    gs_graphics_renderpass_begin(&bake_cb, g_app.face_pass);
    gs_graphics_set_viewport(&bake_cb, 0, 0, w, h);
    gs_graphics_clear(&bake_cb, &clear_desc);
    gsi_draw(gsi, &bake_cb);       // replays the recorded gsi_* commands above
    gs_graphics_renderpass_end(&bake_cb);
    gs_graphics_command_buffer_submit(&bake_cb);
}

void app_init()
{
    g_app.cb = gs_command_buffer_new();

    // ---- Quad geometry (centered at origin, in the XY plane) ----
    vert_t verts[] = {
        {-0.5f, -0.5f, 0.f,  0.f, 0.f},
        { 0.5f, -0.5f, 0.f,  1.f, 0.f},
        { 0.5f,  0.5f, 0.f,  1.f, 1.f},
        {-0.5f,  0.5f, 0.f,  0.f, 1.f},
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

    g_app.vbo = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t){
            .data = verts,
            .size = sizeof(verts)
        }
    );

    g_app.ibo = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t){
            .data = indices,
            .size = sizeof(indices)
        }
    );

    // ---- Shader ----
    gs_graphics_shader_source_desc_t sources[] = {
        {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX,   .source = v_src},
        {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = f_src}
    };
    g_app.shader = gs_graphics_shader_create(
        &(gs_graphics_shader_desc_t){
            .sources = sources,
            .size = sizeof(sources),
            .name = "card_shader"
        }
    );

    // ---- Pipeline (vertex layout must match the vert_t struct) ----
    gs_graphics_vertex_attribute_desc_t layout[] = {
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv"}
    };
    g_app.pip = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t){
            .raster = {
                .shader = g_app.shader,
                .primitive = GS_GRAPHICS_PRIMITIVE_TRIANGLES,
                .index_buffer_element_size = sizeof(uint16_t)
            },
            .blend = {
                .func = GS_GRAPHICS_BLEND_EQUATION_ADD,
                .src  = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
                .dst  = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA
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

    // ---- Uniforms ----
    gs_graphics_uniform_layout_desc_t mvp_layout = {.type = GS_GRAPHICS_UNIFORM_MAT4};
    g_app.u_mvp = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_mvp",
            .stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
            .layout = &mvp_layout,
            .layout_size = sizeof(mvp_layout)
        }
    );

    gs_graphics_uniform_layout_desc_t tex_layout = {.type = GS_GRAPHICS_UNIFORM_SAMPLER2D};
    g_app.u_tex = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_tex",
            .stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
            .layout = &tex_layout,
            .layout_size = sizeof(tex_layout)
        }
    );

    // ---- Load the base card art from disk ----
    // Put a "card.png" next to your executable, or change the path.
    gs_graphics_texture_desc_t tex_desc = gs_default_val();
    int32_t tex_w = 0, tex_h = 0;
    uint32_t num_comps = 0;
    void* tex_data = NULL;
    bool32_t ok = gs_util_load_texture_data_from_file(
        "assets/card.png",
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
        g_app.art_tex = gs_graphics_texture_create(&tex_desc);
        gs_free(tex_data);
    } else {
        gs_println("WARNING: failed to load assets/card.png");
    }

    // ---- Set up the offscreen "face" texture (art + text baked together) ----
    g_app.face_w = (uint32_t)tex_w > 0 ? (uint32_t)tex_w : 512;
    g_app.face_h = (uint32_t)tex_h > 0 ? (uint32_t)tex_h : 716;

    g_app.face_tex = gs_graphics_texture_create(
        &(gs_graphics_texture_desc_t){
            .width = g_app.face_w,
            .height = g_app.face_h,
            .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
            .min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
            .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
            .data = {NULL} // no initial data -- we render into it below
        }
    );

    g_app.face_fbo = gs_graphics_framebuffer_create(&(gs_graphics_framebuffer_desc_t){0});
    g_app.face_pass = gs_graphics_renderpass_create(
        &(gs_graphics_renderpass_desc_t){
            .fbo = g_app.face_fbo,
            .color = &g_app.face_tex,
            .color_size = sizeof(g_app.face_tex)
        }
    );

    g_app.gsi = gs_immediate_draw_new();

    // ---- Load fonts at the point sizes you want ----
    // Put a .ttf next to your executable, or change the path.
    // Bump point_size up/down to make this text bigger/smaller.
    if (!gs_asset_font_load_from_file("assets/font.otf", &g_app.font_title, 48)) {
        gs_println("WARNING: failed to load assets/font.ttf (title, 48pt)");
    }
    if (!gs_asset_font_load_from_file("assets/font.otf", &g_app.font_label, 24)) {
        gs_println("WARNING: failed to load assets/font.ttf (label, 24pt)");
    }

    // ---- Camera: perspective, pulled back so we can see the card ----
    g_app.cam = gs_camera_perspective();
    g_app.cam.transform.position = gs_v3(0.f, 0.f, 3.f);

    // ---- Card's world transform ----
    g_app.card_xform = gs_vqs_default();

    // The quad mesh above is a fixed 1:1 square, but a real card texture
    // usually isn't square (e.g. a portrait card is narrower than it is
    // tall). Correct for that here by scaling X to match the texture's
    // aspect ratio, so the art isn't stretched to fill the square mesh.
    if (g_app.face_h > 0) {
        float aspect = (float)g_app.face_w / (float)g_app.face_h;
        g_app.card_xform.scale.x = aspect;
    }

    // Bake the art + text into face_tex once at startup.
    // Call this again any time the card's text/label changes.
    app_bake_card_face(ok, "ACE OF SPADES", "1");
}

void app_update()
{
    if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
        gs_quit();
    }

    float dt = gs_platform_delta_time();

    // ---- Move the card with WASD (X/Y plane) ----
    float speed = 2.f * dt;
    if (gs_platform_key_down(GS_KEYCODE_A)) g_app.card_xform.position.x -= speed;
    if (gs_platform_key_down(GS_KEYCODE_D)) g_app.card_xform.position.x += speed;
    if (gs_platform_key_down(GS_KEYCODE_S)) g_app.card_xform.position.y -= speed;
    if (gs_platform_key_down(GS_KEYCODE_W)) g_app.card_xform.position.y += speed;

    // ---- Rotate the card with Q/E (around Y) and Z/C (around X) ----
    float rot_speed = 90.f * dt; // degrees/sec
    if (gs_platform_key_down(GS_KEYCODE_Q)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(0, 1, 0));
        g_app.card_xform.rotation = gs_quat_mul(dq, g_app.card_xform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_E)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(0, 1, 0));
        g_app.card_xform.rotation = gs_quat_mul(dq, g_app.card_xform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_Z)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(rot_speed), gs_v3(1, 0, 0));
        g_app.card_xform.rotation = gs_quat_mul(dq, g_app.card_xform.rotation);
    }
    if (gs_platform_key_down(GS_KEYCODE_C)) {
        gs_quat dq = gs_quat_angle_axis(gs_deg2rad(-rot_speed), gs_v3(1, 0, 0));
        g_app.card_xform.rotation = gs_quat_mul(dq, g_app.card_xform.rotation);
    }

    // ---- Build MVP ----
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);

    gs_mat4 model = gs_vqs_to_mat4(&g_app.card_xform);
    gs_mat4 vp    = gs_camera_get_view_projection(&g_app.cam, (int32_t)fbw, (int32_t)fbh);
    gs_mat4 mvp   = gs_mat4_mul(vp, model);

    // ---- Render ----
    gs_command_buffer_t* cb = &g_app.cb;

    gs_graphics_clear_action_t clear = {.flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH, .color = {0.1f, 0.1f, 0.15f, 1.f}};
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(cb, GS_GRAPHICS_RENDER_PASS_DEFAULT); // default (screen) render pass
    gs_graphics_clear(cb, &clear_desc);
    gs_graphics_set_viewport(cb, 0, 0, fbw, fbh);

    gs_graphics_pipeline_bind(cb, g_app.pip);

    gs_graphics_bind_vertex_buffer_desc_t vbuf = {.buffer = g_app.vbo};
    gs_graphics_bind_index_buffer_desc_t  ibuf = {.buffer = g_app.ibo};
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        {.uniform = g_app.u_mvp, .data = &mvp,           .binding = 0},
        {.uniform = g_app.u_tex, .data = &g_app.face_tex, .binding = 0}
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

gs_app_desc_t gs_main(int32_t argc, char** argv)
{
    return (gs_app_desc_t){
        .init = app_init,
        .update = app_update,
        .window = { .title = "Card Game - Textured Quad",
            .width = 1280,
            .height = 720
        }
    };
}
