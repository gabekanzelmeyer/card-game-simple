#define GS_IMPL
#include "gs.h"

#define GS_IMMEDIATE_DRAW_IMPL
#include "util/gs_idraw.h"

#define CARD_TEXTURE_WIDTH 500
#define CARD_TEXTURE_HEIGHT 800

typedef struct {
    gs_handle(gs_graphics_pipeline_t) card_pipeline;
    gs_handle(gs_graphics_vertex_buffer_t) card_vertex_buffer;
    gs_handle(gs_graphics_index_buffer_t) card_index_buffer;
    gs_handle(gs_graphics_shader_t) card_shader;
    gs_handle(gs_graphics_texture_t) card_texture;
    gs_asset_font_t card_font;
} card_global_state;

typedef struct {
    const char *name;
    uint16_t attack;
    uint16_t health;

    gs_vqs transform;

    gs_handle(gs_graphics_uniform_t) uniform_mvp;
    gs_handle(gs_graphics_uniform_t) uniform_texture;

    gs_handle(gs_graphics_texture_t) texture;
    gs_handle(gs_graphics_framebuffer_t) framebuffer;
    gs_handle(gs_graphics_renderpass_t) renderpass;
} card_state;

typedef struct {
    float x, y, z, u, v;
} card_vertex;

static const char* card_vertex_shader_src =
"#version 330 core\n"
"layout(location = 0) in vec3 a_pos;\n"
"layout(location = 1) in vec2 a_uv;\n"
"uniform mat4 u_mvp;\n"
"out vec2 uv;\n"
"void main() {\n"
"   uv = a_uv;\n"
"   gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
"}\n";

static const char* card_fragment_shader_src =
"#version 330 core\n"
"in vec2 uv;\n"
"out vec4 frag_color;\n"
"uniform sampler2D u_tex;\n"
"void main() {\n"
"   frag_color = texture(u_tex, uv);\n"
"}\n";

card_global_state card_global = {0};

void card_global_init() {
    // common vertices and indices used for all cards
    card_vertex verts[] = {
        {-0.5f, -0.5f, 0.f,  0.f, 0.f},
        { 0.5f, -0.5f, 0.f,  1.f, 0.f},
        { 0.5f,  0.5f, 0.f,  1.f, 1.f},
        {-0.5f,  0.5f, 0.f,  0.f, 1.f},
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

    card_global.card_vertex_buffer = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = verts,
            .size = sizeof(verts)
        }
    );

    card_global.card_index_buffer = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t) {
            .data = indices,
            .size = sizeof(indices)
        }
    );

    // create the card shader
    gs_graphics_shader_source_desc_t sources[] = {
        {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = card_vertex_shader_src},
        {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = card_fragment_shader_src}
    };
    card_global.card_shader = gs_graphics_shader_create(
        &(gs_graphics_shader_desc_t){
            .sources = sources,
            .size = sizeof(sources),
            .name = "card_shader"
        }
    );

    // create the pipeline to draw the cards
    gs_graphics_vertex_attribute_desc_t pipeline_layout[] = {
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv"}
    };
    card_global.card_pipeline = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t){
            .raster = {
                .shader = card_global.card_shader,
                .primitive = GS_GRAPHICS_PRIMITIVE_TRIANGLES,
                .index_buffer_element_size = sizeof(uint16_t)
            },
            .blend = {
                .func = GS_GRAPHICS_BLEND_EQUATION_ADD,
                .src  = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
                .dst  = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA
            },
            .layout = {
                .attrs = pipeline_layout,
                .size = sizeof(pipeline_layout)
            },
            .depth = {
                .func = GS_GRAPHICS_DEPTH_FUNC_LESS
            }
        }
    );

    // load base texture image
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

    // load texture
    if (ok) {
        gs_graphics_texture_desc_t base_texture_desc = gs_default_val();
        base_texture_desc.width = (uint32_t)tex_w;
        base_texture_desc.height = (uint32_t)tex_h;
        base_texture_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
        base_texture_desc.data[0] = tex_data;
        base_texture_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
        base_texture_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
        card_global.card_texture = gs_graphics_texture_create(&base_texture_desc);
        gs_free(tex_data);
    } else {
        gs_println("WARNING: failed to load assets/card.png");
    }

    // load font
    if (!gs_asset_font_load_from_file("assets/font.otf", &card_global.card_font, 100)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }
}

card_state card_new(const char *name,
                 uint16_t attack,
                 uint16_t health) {
    card_state card = {0};
    card.name = name;
    card.attack = attack;
    card.health = health;
    card.transform = gs_vqs_default();

    gs_graphics_uniform_layout_desc_t mvp_layout = {.type = GS_GRAPHICS_UNIFORM_MAT4};
    card.uniform_mvp = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_mvp", // the name of the shader uniform variable
            .stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
            .layout = &mvp_layout,
            .layout_size = sizeof(mvp_layout)
        }
    );

    gs_graphics_uniform_layout_desc_t texture_layout = {.type = GS_GRAPHICS_UNIFORM_SAMPLER2D};
    card.uniform_texture = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_tex", // the name of the shader uniform variable
            .stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
            .layout = &texture_layout,
            .layout_size = sizeof(texture_layout)
        }
    );

    card.texture = gs_graphics_texture_create(
        &(gs_graphics_texture_desc_t){
            .width = CARD_TEXTURE_WIDTH,
            .height = CARD_TEXTURE_HEIGHT,
            .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
            .min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
            .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
            .data = {NULL} // no data it gets rendered with card_render
        }
    );

    card.framebuffer = gs_graphics_framebuffer_create(&(gs_graphics_framebuffer_desc_t){0});
    card.renderpass = gs_graphics_renderpass_create(
        &(gs_graphics_renderpass_desc_t){
            .fbo = card.framebuffer,
            .color = &card.texture,
            .color_size = sizeof(card.texture)
        }
    );

    return card;
}

void card_bake_texture(card_state *card, gs_immediate_draw_t *immediate_draw) {
    // set 2d orthographic projection the size of the card face
    gsi_camera2D(immediate_draw, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);
    // draw the card texture (template face texture for now) fit / stretched onto the card's face texture'
    gsi_texture(immediate_draw, card_global.card_texture);
    gsi_rectvd(immediate_draw,
               gs_v2(0.f, 0.f),
               gs_v2(CARD_TEXTURE_WIDTH, CARD_TEXTURE_WIDTH),
               gs_v2(0.f, 0.f),
               gs_v2(1.f, 1.f),
               GS_COLOR_WHITE,
               GS_GRAPHICS_PRIMITIVE_TRIANGLES);
    // draw the card name
    gsi_text(immediate_draw, 24.f, 24.f, card->name, &card_global.card_font, false, 20, 20, 20, 255);

    // submit an offscreen render pass to draw the face to the card texture
    gs_command_buffer_t command_buffer = gs_command_buffer_new();
    gs_graphics_clear_action_t clear = {
        .flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH,
        .color = {0.f, 0.f, 0.f, 0.f} // fully transparent so any leftover alpha in the art shows through
    };
    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_renderpass_begin(&command_buffer, card->renderpass);
    gs_graphics_set_viewport(&command_buffer, 0, 0, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);
    gs_graphics_clear(&command_buffer, &clear_desc);
    gsi_draw(immediate_draw, &command_buffer);       // replays the recorded gsi_* commands above
    gs_graphics_renderpass_end(&command_buffer);
    gs_graphics_command_buffer_submit(&command_buffer);
}
