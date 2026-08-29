#ifndef UTIL_CARD_H
#define UTIL_CARD_H

#include "gs.h"
#include "util/gs_idraw.h"

#define CARD_TEXTURE_WIDTH  600
#define CARD_TEXTURE_HEIGHT 800

#define CARD_ATLAS_MAX_SLOTS 16


typedef struct {
    int shield; // when damaged, prevent damage and reduce count
    int strike; // deal X damage to target
    int heal; // increase target health by X
    int blunt; // reduce target attack by X
    int sharpen; // increase target attack by X

    int regenerate; // if the card is destroyed, reduce count add to hand
    int wellspring; // all hand cards get +X health
    int warcry; // all hand cards get +X attack
    int swipe; // deal X damage to all opp hand cards
    int disarm; // reduce opposing card attack by X (no less than 1)
    int cripple; // reduce opponent hand cards attack by X (no less than 1)
    int channel; // all cards in hands and in play get +X/+X if they share this cards color
    int sacrifice; // destroy X cards in hand
    int assassinate; // destroy X cards in opp hand
    bool haste; // attacks early in battle
    bool timebound; // if a cards with timebound is in hand, they must be played before other cards
} card_abilities_t;

typedef struct {
    const char *name;
    uint16_t attack;
    uint16_t health;
    bool red;
    bool green;
    bool blue;
    card_abilities_t abilities;
    int current_attack;
    int current_health;
    card_abilities_t current_abilities;

    bool selectable;
    bool hovered;
    gs_vqs transform;

    float anim_duration;
    float lerp;
    gs_vqs prev_transform;
    gs_vqs target_transform;

    uint32_t render_index;
} card_state_t;

typedef struct {
    float x, y, z, u, v;
} card_vertex_data_t;

typedef struct {
    float mvp[16];
    float uv_rect[4];
} card_instance_data_t;

typedef struct {
    gs_handle(gs_graphics_pipeline_t) card_pipeline;
    gs_handle(gs_graphics_vertex_buffer_t) card_vertex_buffer;
    gs_handle(gs_graphics_vertex_buffer_t) card_instance_buffer;
    gs_handle(gs_graphics_index_buffer_t) card_index_buffer;
    gs_handle(gs_graphics_shader_t) card_shader;
    gs_handle(gs_graphics_texture_t) card_atlas_texture;

    gs_handle(gs_graphics_uniform_t) uniform_render_target_texture;
    gs_handle(gs_graphics_texture_t) render_target_texture;
    gs_handle(gs_graphics_framebuffer_t) render_target_framebuffer;
    gs_handle(gs_graphics_renderpass_t) render_target_renderpass;

    gs_asset_font_t card_name_font;
    gs_asset_font_t card_stats_font;
    gs_asset_font_t card_abilities_font;

    gs_hash_table(const char*, card_state_t) card_lookup;
} card_util_t;

void card_util_init();
card_state_t card_new(const char *name, uint16_t attack, uint16_t health, bool red, bool green, bool blue, card_abilities_t abilities);
void card_reset_stats(card_state_t *card);
card_state_t card_get_random(uint32_t render_index);
void card_update_visuals(card_state_t *card, gs_immediate_draw_t *immediate_draw);
void cards_render_instanced(card_state_t* cards, uint32_t count, gs_command_buffer_t* command_buffer, gs_mat4 view_projection);
bool card_do_colors_match(card_state_t card1, card_state_t card2);
bool card_has_target_ability(card_state_t *card);

#ifdef UTIL_CARD_IMPL

card_util_t card_util = {0};

char* load_file(const char* filename) {
    char *buffer = 0;
    long length;
    FILE *f = fopen(filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length + 1);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }
    buffer[length] = '\0';
    return buffer;
}

card_state_t card_new(const char *name,
                      uint16_t attack,
                      uint16_t health,
                      bool red,
                      bool green,
                      bool blue,
                      card_abilities_t abilities) {
    card_state_t card = {0};
    card.name = name;
    card.attack = attack;
    card.health = health;
    card.red = red;
    card.green = green;
    card.blue = blue;
    card.abilities = abilities;
    card_reset_stats(&card);
    card.transform = gs_vqs_default();
    return card;
}

bool card_do_colors_match(card_state_t card1, card_state_t card2) {
    if (card1.red && card2.red) return true;
    if (card1.green && card2.green) return true;
    if (card1.blue && card2.blue) return true;
    return false;
}

bool card_has_target_ability(card_state_t *card) {
    return card->abilities.strike > 0
    || card->abilities.blunt > 0
    || card->abilities.heal > 0
    || card->abilities.sharpen > 0;
}

static void card_add_to_lookup(card_state_t card) {
    gs_hash_table_insert(card_util.card_lookup, card.name, card);
}

static void card_populate_lookup() {
    card_add_to_lookup(card_new("Barbtail", 1, 2, true, false, false, (card_abilities_t){.strike=1}));
    card_add_to_lookup(card_new("Pikeling", 3, 1, true, false, false, (card_abilities_t){.sharpen=1}));
    card_add_to_lookup(card_new("Tristeed", 2, 3, true, false, false, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Pyramount", 3, 2, true, false, false, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Anglewing", 1, 2, true, false, false, (card_abilities_t){.haste=true}));
    card_add_to_lookup(card_new("Spireback", 1, 4, true, false, false, (card_abilities_t){0}));

    card_add_to_lookup(card_new("Blockheart", 2, 2, false, true, false, (card_abilities_t){.heal=1}));
    card_add_to_lookup(card_new("Squareback", 2, 3, false, true, false, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Boxling", 2, 1, false, true, false, (card_abilities_t){.regenerate=1}));
    card_add_to_lookup(card_new("Bullwark", 1, 4, false, true, false, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Cubeface", 3, 3, false, true, false, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Quadlet", 1, 3, false, true, false, (card_abilities_t){.heal=1}));

    card_add_to_lookup(card_new("Orbling", 2, 2, false, false, true, (card_abilities_t){.blunt=1}));
    card_add_to_lookup(card_new("Loopwing", 4, 2, false, false, true, (card_abilities_t){.timebound=true}));
    card_add_to_lookup(card_new("Circlet", 2, 3, false, false, true, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Ringform", 1, 3, false, false, true, (card_abilities_t){.blunt=1}));
    card_add_to_lookup(card_new("Roundshell", 1, 4, false, false, true, (card_abilities_t){0}));
    card_add_to_lookup(card_new("Spheric", 3, 2, false, false, true, (card_abilities_t){0}));
    // card_add_to_lookup(card_new("Sunfish", 5, 5, false, false, true, (card_abilities_t){0}));
    // card_add_to_lookup(card_new("Immortal Jelly", 2, 1, false, false, true, (card_abilities_t){.regenerate=3}));
    // card_add_to_lookup(card_new("Kraken", 4, 4, false, false, true, (card_abilities_t){.channel=2}));

    // card_add_to_lookup(card_new("Slab", 2, 4, true, false, false, (card_abilities_t){.shield = 1}));
    // card_add_to_lookup(card_new("Tusk", 5, 3, true, false, false, (card_abilities_t){0}));
    // card_add_to_lookup(card_new("Iron", 3, 3, true, false, false, (card_abilities_t){.wellspring = 2}));
    // card_add_to_lookup(card_new("Cinder", 3, 1, true, false, false, (card_abilities_t){.haste = true, .strike = 1}));
    // card_add_to_lookup(card_new("Basalt", 3, 4, true, false, false, (card_abilities_t){.swipe = 1}));
    // card_add_to_lookup(card_new("Rust", 1, 4, true, false, false, (card_abilities_t){.warcry = 1}));
    // card_add_to_lookup(card_new("Bramble", 2, 3, false, true, false, (card_abilities_t){.regenerate = 1}));
    // card_add_to_lookup(card_new("Demon", 6, 6, false, true, false, (card_abilities_t){.sacrifice = 1}));
    // card_add_to_lookup(card_new("Theif", 2, 2, false, true, false, (card_abilities_t){.assassinate = 1}));
    // card_add_to_lookup(card_new("Clockwork", 5, 5, false, false, true, (card_abilities_t){.timebound = true}));
    // card_add_to_lookup(card_new("Mage", 3, 3, false, false, true, (card_abilities_t){.disarm = 2}));
    // card_add_to_lookup(card_new("Wizard", 1, 4, false, false, true, (card_abilities_t){.cripple = 2}));
    // card_add_to_lookup(card_new("Master", 1, 4, false, false, true, (card_abilities_t){.channel = 2}));
}

void card_reset_stats(card_state_t *card) {
    card->current_attack = card->attack;
    card->current_health = card->health;
    card->current_abilities = card->abilities;
}

void card_util_init() {
    // shared verts / indices used by every card instance
    float card_width_ratio = (float)CARD_TEXTURE_WIDTH / (float)CARD_TEXTURE_HEIGHT;
    card_vertex_data_t verts[] = {
        {-card_width_ratio, -1.0f, 0.f,  0.f, 0.f},
        {card_width_ratio, -1.0f, 0.f,  1.f, 0.f},
        {card_width_ratio,  1.0f, 0.f,  1.f, 1.f},
        {-card_width_ratio,  1.0f, 0.f,  0.f, 1.f},
    };
    uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

    card_util.card_vertex_buffer = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = verts,
            .size = sizeof(verts)
        }
    );
    card_util.card_index_buffer = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t) {
            .data = indices,
            .size = sizeof(indices)
        }
    );
    // create the per-card instance data buffer
    card_util.card_instance_buffer = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = NULL,
            .size = CARD_ATLAS_MAX_SLOTS * sizeof(card_instance_data_t),
            .usage = GS_GRAPHICS_BUFFER_USAGE_STREAM
        }
    );

    gs_graphics_shader_source_desc_t sources[] = {
        {.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = load_file("shaders/card.vert")},
        {.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = load_file("shaders/card.frag")}
    };
    card_util.card_shader = gs_graphics_shader_create(
        &(gs_graphics_shader_desc_t){
            .sources = sources,
            .size = sizeof(sources),
            .name = "card_shader"
        }
    );

    // set up the pipeline, .divisor = 0 means per vertex, .divisor = 1 means per instance
    gs_graphics_vertex_attribute_desc_t pipeline_layout[] = {
        // per-vertex (divisor = 0): stride = sizeof(card_vertex) = 20 bytes
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos", .stride = sizeof(card_vertex_data_t), .offset = 0, .divisor = 0},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv", .stride = sizeof(card_vertex_data_t), .offset = 12, .divisor = 0},
        // per-instance (divisor = 1): stride = sizeof(card_instance_t) = 80 bytes
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4, .name = "a_mvp0", .stride = sizeof(card_instance_data_t), .offset = 0, .divisor = 1},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4, .name = "a_mvp1", .stride = sizeof(card_instance_data_t), .offset = 16, .divisor = 1},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4, .name = "a_mvp2", .stride = sizeof(card_instance_data_t), .offset = 32, .divisor = 1},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4, .name = "a_mvp3", .stride = sizeof(card_instance_data_t), .offset = 48, .divisor = 1},
        {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT4, .name = "a_uv_rect", .stride = sizeof(card_instance_data_t), .offset = 64, .divisor = 1},
    };
    card_util.card_pipeline = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t){
            .raster = {
                .shader = card_util.card_shader,
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
                .func = GS_GRAPHICS_DEPTH_FUNC_LESS,
                .mask = GS_GRAPHICS_DEPTH_MASK_DISABLED
            }
        }
    );

    // setup the uniform for the card render target texture (shared by all card instances)
    gs_graphics_uniform_layout_desc_t texture_layout = {.type = GS_GRAPHICS_UNIFORM_SAMPLER2D};
    card_util.uniform_render_target_texture = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_tex",
            .stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
            .layout = &texture_layout,
            .layout_size = sizeof(texture_layout)
        }
    );

    // load the atlas texture
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
    if (ok) {
        gs_graphics_texture_desc_t base_texture_desc = gs_default_val();
        base_texture_desc.width = (uint32_t)tex_w;
        base_texture_desc.height = (uint32_t)tex_h;
        base_texture_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
        base_texture_desc.data[0] = tex_data;
        base_texture_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
        base_texture_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
        card_util.card_atlas_texture = gs_graphics_texture_create(&base_texture_desc);
        gs_free(tex_data);
    } else {
        gs_println("WARNING: failed to load assets/card.png");
    }

    // load the fonts
    if (!gs_asset_font_load_from_file("assets/font.otf", &card_util.card_name_font, 120)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }
    if (!gs_asset_font_load_from_file("assets/font.otf", &card_util.card_stats_font, 140)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }
    if (!gs_asset_font_load_from_file("assets/font.otf", &card_util.card_abilities_font, 80)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }

    // create the render target texture, each card gets rendered to a specific position in this texture
    card_util.render_target_texture = gs_graphics_texture_create(
        &(gs_graphics_texture_desc_t){
            .width = CARD_TEXTURE_WIDTH,
            .height = CARD_TEXTURE_HEIGHT * CARD_ATLAS_MAX_SLOTS,
            .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
            .min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
            .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
            .data = {NULL} // rendered into below, per-card, via card_bake_texture
        }
    );
    card_util.render_target_framebuffer = gs_graphics_framebuffer_create(&(gs_graphics_framebuffer_desc_t){0});
    card_util.render_target_renderpass = gs_graphics_renderpass_create(
        &(gs_graphics_renderpass_desc_t){
            .fbo = card_util.render_target_framebuffer,
            .color = &card_util.render_target_texture,
            .color_size = sizeof(card_util.render_target_texture)
        }
    );

    // add cards to lookup table
    card_populate_lookup();
}

card_state_t card_get_random(uint32_t render_index) {
    // in order to populate hands with a random card, put the lookup map in a list and get a random index
    gs_dyn_array(card_state_t) lookup_cards = NULL;
    for (gs_hash_table_iter it = gs_hash_table_iter_new(card_util.card_lookup);
         gs_hash_table_iter_valid(card_util.card_lookup, it);
    gs_hash_table_iter_advance(card_util.card_lookup, it)) {
        gs_dyn_array_push(lookup_cards, gs_hash_table_iter_get(card_util.card_lookup, it));
    }

    int random_index = (rand() % gs_dyn_array_size(lookup_cards));
    card_state_t card = lookup_cards[random_index];
    card.render_index = render_index;
    gs_dyn_array_free(lookup_cards);
    return card;
}

// Returns this card's row within the shared atlas, in normalized UV space.
static gs_vec4_t card_render_target_uv_rect(const card_state_t* card) {
    float uv_height = 1.f / (float)CARD_ATLAS_MAX_SLOTS;
    float y_offset = (float) card->render_index * uv_height;
    gs_vec4_t r;
    r.x = 0.f;
    r.y = y_offset;
    r.z = 1.f;
    r.w = uv_height;
    return r;
}

void card_update_visuals(card_state_t *card, gs_immediate_draw_t *immediate_draw) {
    uint32_t y_offset = (float)card->render_index * CARD_TEXTURE_HEIGHT;

    gsi_camera2D(immediate_draw, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);

    gsi_texture(immediate_draw, card_util.card_atlas_texture);

    gsi_rectvd(immediate_draw,
        gs_v2(0.f, 0.f),
        gs_v2(CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT),
        gs_v2(0.f, 0.f), gs_v2(1.f, 1.f),
        gs_color(50, card->selectable ? 255 : 50, 50, 255),
        GS_GRAPHICS_PRIMITIVE_TRIANGLES);

    // TODO: this will change once more textures are added to the atlas
    gsi_rectvd(immediate_draw,
        gs_v2(12.f, 12.f),
        gs_v2(CARD_TEXTURE_WIDTH - 24, CARD_TEXTURE_HEIGHT - 24),
        gs_v2(0.f, 0.f),
        gs_v2(1.f, 1.f),
        GS_COLOR_WHITE,
        GS_GRAPHICS_PRIMITIVE_TRIANGLES);


    gs_vec2 text_dimensions = gs_asset_font_text_dimensions(&card_util.card_name_font, card->name, -1); // -1 means null-terminated string

    gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, 24.f, card->name,
             &card_util.card_name_font, false, card->red ? 200 : 20, card->green ? 200 : 20, card->blue ? 200 : 20, 255);

    float ability_y_offset = 400.0f;
    float offset_increment = 60.f;
    if (card->current_abilities.shield > 0) {
        char ability_buffer[20] = "Shield ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.shield);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.strike > 0) {
        char ability_buffer[20] = "Strike ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.strike);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.heal > 0) {
        char ability_buffer[20] = "Heal ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.heal);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.blunt > 0) {
        char ability_buffer[20] = "Blunt ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.blunt);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.sharpen > 0) {
        char ability_buffer[20] = "Sharpen ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.sharpen);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }


    if (card->current_abilities.disarm > 0) {
        char ability_buffer[20] = "Disarm ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.disarm);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.regenerate) {
        char ability_buffer[20] = "Regenerate ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.regenerate);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.wellspring > 0) {
        char ability_buffer[20] = "Wellspring ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.wellspring);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.warcry > 0) {
        char ability_buffer[20] = "Warcry ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.warcry);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.swipe > 0) {
        char ability_buffer[20] = "Swipe ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.swipe);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.cripple > 0) {
        char ability_buffer[20] = "Cripple ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.cripple);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.channel > 0) {
        char ability_buffer[20] = "Channel ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.channel);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.sacrifice > 0) {
        char ability_buffer[20] = "Sacrifice ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.sacrifice);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.assassinate > 0) {
        char ability_buffer[20] = "Assassinate ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card->current_abilities.assassinate);
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.haste) {
        char ability_buffer[20] = "Haste";
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card->current_abilities.timebound) {
        char ability_buffer[20] = "Timebound";
        text_dimensions = gs_asset_font_text_dimensions(&card_util.card_abilities_font, ability_buffer, strlen(ability_buffer));
        gsi_text(immediate_draw, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &card_util.card_abilities_font, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }

    char attack_char_buffer[10];
    snprintf(attack_char_buffer, sizeof(attack_char_buffer), "%d", card->current_attack);
    gsi_text(immediate_draw, 50.f, 650.f, attack_char_buffer, &card_util.card_stats_font, false, card->current_attack < card->attack ? 255 : 20, card->current_attack > card->attack ? 255 : 20, 20, 255);

    char health_char_buffer[10];
    snprintf(health_char_buffer, sizeof(health_char_buffer), "%d", card->current_health);
    text_dimensions = gs_asset_font_text_dimensions(&card_util.card_stats_font, health_char_buffer, strlen(health_char_buffer));
    gsi_text(immediate_draw, CARD_TEXTURE_WIDTH - text_dimensions.x - 50.f, 650.f, health_char_buffer, &card_util.card_stats_font, false, card->current_health < card->health ? 255 : 20, card->current_health > card->health ? 255 : 20, 20, 255);

    gs_command_buffer_t command_buffer = gs_command_buffer_new();
    gs_graphics_clear_action_t clear = {
        .flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH,
        .color = {0.f, 0.f, 0.f, 0.f}
    };

    gs_graphics_renderpass_begin(&command_buffer, card_util.render_target_renderpass);

    // set viewport and sizor to this cards position in the render target
    // scissor makes sure the clear doesn't wipe out other already-baked textures
    gs_graphics_set_viewport(&command_buffer, 0, y_offset, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);
    gs_graphics_set_view_scissor(&command_buffer, 0, y_offset, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);

    gs_graphics_clear_desc_t clear_desc = {.actions = &clear, .size = sizeof(clear)};
    gs_graphics_clear(&command_buffer, &clear_desc);

    gsi_draw(immediate_draw, &command_buffer);
    gs_graphics_renderpass_end(&command_buffer); // also disables scissor test automatically

    gs_graphics_command_buffer_submit(&command_buffer);
}

void cards_render_instanced(card_state_t* cards,
                            uint32_t count,
                            gs_command_buffer_t* command_buffer,
                            gs_mat4 view_projection) {
    if (count == 0) return;
    if (count > CARD_ATLAS_MAX_SLOTS) count = CARD_ATLAS_MAX_SLOTS; // safety clamp

    // ---- Build the per-instance CPU buffer ----
    static card_instance_data_t instance_data[CARD_ATLAS_MAX_SLOTS];
    for (uint32_t i = 0; i < count; ++i) {
        gs_mat4 model = gs_vqs_to_mat4(&cards[i].transform);
        gs_mat4 mvp = gs_mat4_mul(view_projection, model);
        memcpy(instance_data[i].mvp, mvp.elements, sizeof(instance_data[i].mvp));

        float uv_height = 1.f / (float)CARD_ATLAS_MAX_SLOTS;
        float y_offset = (float)cards[i].render_index * uv_height;

        gs_vec4 r = card_render_target_uv_rect(&cards[i]);
        instance_data[i].uv_rect[0] = r.x;
        instance_data[i].uv_rect[1] = r.y;
        instance_data[i].uv_rect[2]  = r.z;
        instance_data[i].uv_rect[3]  = r.w;
    }

    // update the per instance data buffer
    gs_graphics_vertex_buffer_request_update(
        command_buffer,
        card_util.card_instance_buffer,
        &(gs_graphics_vertex_buffer_desc_t) {
            .data = instance_data,
            .size = count * sizeof(card_instance_data_t),
            .usage = GS_GRAPHICS_BUFFER_USAGE_STREAM
        }
    );

    gs_graphics_pipeline_bind(command_buffer, card_util.card_pipeline);

    // 7 entries, matching the 7 attributes in the pipeline layout (must be in order)
    gs_graphics_bind_vertex_buffer_desc_t vbufs[] = {
        {.buffer = card_util.card_vertex_buffer},   // a_pos
        {.buffer = card_util.card_vertex_buffer},   // a_uv
        {.buffer = card_util.card_instance_buffer}, // a_mvp0
        {.buffer = card_util.card_instance_buffer}, // a_mvp1
        {.buffer = card_util.card_instance_buffer}, // a_mvp2
        {.buffer = card_util.card_instance_buffer}, // a_mvp3
        {.buffer = card_util.card_instance_buffer}, // a_uv_rect
    };
    gs_graphics_bind_index_buffer_desc_t ibuf = {.buffer = card_util.card_index_buffer};
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        {
            .uniform = card_util.uniform_render_target_texture,
            .data = &card_util.render_target_texture,
            .binding = 0
        }
    };
    gs_graphics_bind_desc_t binds = {
        .vertex_buffers = {.desc = vbufs, .size = sizeof(vbufs)},
        .index_buffers  = {.desc = &ibuf},
        .uniforms       = {.desc = uniforms, .size = sizeof(uniforms)}
    };
    gs_graphics_apply_bindings(command_buffer, &binds);

    gs_graphics_draw(command_buffer, &(gs_graphics_draw_desc_t) {
        .start = 0,
        .count = 6,
        .instances = count
    });
}
#endif // UTIL_CARD_IMPL
#endif // UTIL_CARD_H
