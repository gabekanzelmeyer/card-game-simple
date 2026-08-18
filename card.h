#define GS_IMPL
#include "gs.h"

#define CARD_TEXTURE_WIDTH 500
#define CARD_TEXTURE_HEIGHT 800

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

card_state card_create(const char *name,
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

    gs_graphics_uniform_layout_desc_t tex_layout = {.type = GS_GRAPHICS_UNIFORM_SAMPLER2D};
    card.uniform_texture = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_tex", // the name of the shader uniform variable
            .stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
            .layout = &tex_layout,
            .layout_size = sizeof(tex_layout)
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
