#define GS_IMPL
#include "gs.h"

#define GS_IMMEDIATE_DRAW_IMPL
#include "util/gs_idraw.h"

#include "card.h"

typedef struct {
    gs_command_buffer_t command_buffer;
    gs_handle(gs_graphics_pipeline_t) card_pipeline;
    gs_handle(gs_graphics_vertex_buffer_t) card_vertex_buffer;
    gs_handle(gs_graphics_index_buffer_t) card_index_buffer;
    gs_handle(gs_graphics_shader_t) card_shader;
    gs_asset_font_t card_font;
    gs_immediate_draw_t immediate_draw;
    gs_camera_t camera;
} game_state;

static game_state state = {0};

void init() {
    state.command_buffer = gs_command_buffer_new();

    // common vertices and indices used for all cards
    card_vertex verts[] = {
        {-0.5f, -0.5f, 0.f,  0.f, 0.f},
        { 0.5f, -0.5f, 0.f,  1.f, 0.f},
        { 0.5f,  0.5f, 0.f,  1.f, 1.f},
        {-0.5f,  0.5f, 0.f,  0.f, 1.f},
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
    state.card_vertex_buffer = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = verts,
            .size = sizeof(verts)
        }
    );

    state.card_index_buffer = gs_graphics_index_buffer_create(
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
    state.card_shader = gs_graphics_shader_create(
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
    state.card_pipeline = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t){
            .raster = {
                .shader = state.card_shader,
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
}

void update() {

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
