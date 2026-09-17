#ifndef CARD_ENTITY_H
#define CARD_ENTITY_H

#include "gs.h"
#include "util/gs_idraw.h"

#include "engine.h"
#include "engine_shapes.h"
#include "card_data.h"

#define HAND_SPACING 1.2f
#define HAND_FAN_ANGLE 2.0f
#define HAND_CURVE_AMOUNT 0.018f
#define HAND_Y_POSITION_OFFSET 1.9f

typedef struct {
    entity_t entity;
    card_data_t data;
    gs_handle(gs_graphics_framebuffer_t) frame_buffer;
    gs_handle(gs_graphics_renderpass_t) render_pass;
} card_entity_t;

uint32_t CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT;
gs_handle(gs_graphics_texture_t) CARD_BG_TEXTURE;
gs_asset_font_t CARD_FONT;
gs_vec3 CARD_SCALE;


void card_entites_init() {
    int32_t tex_w = 0, tex_h = 0;
    uint32_t num_comps = 0;
    void* tex_data = NULL;
    gs_util_load_texture_data_from_file("assets/card.png", &tex_w, &tex_h, &num_comps, &tex_data, true);

    CARD_TEXTURE_WIDTH = (uint32_t)tex_w;
    CARD_TEXTURE_HEIGHT = (uint32_t)tex_h;

    CARD_SCALE = gs_v3(1.2f, 1.f, 1.6f);

    gs_graphics_texture_desc_t bg_texture_desc = gs_default_val();
    bg_texture_desc.width = (uint32_t)tex_w;
    bg_texture_desc.height = (uint32_t)tex_h;
    bg_texture_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
    bg_texture_desc.data[0] = tex_data;
    bg_texture_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    bg_texture_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;

    CARD_BG_TEXTURE = gs_graphics_texture_create(&bg_texture_desc);
    gs_free(tex_data);

    if (!gs_asset_font_load_from_file("assets/font.otf", &CARD_FONT, 100)) {
        gs_println("WARNING: failed to load assets/font.otf (100pt)");
    }
}

card_entity_t card_entity_create(card_data_t card_data) {
    render_texture_t render_texture = render_texture_create(CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);
    gs_graphics_framebuffer_desc_t fb_desc = gs_default_val();
    gs_handle(gs_graphics_framebuffer_t) frame_buffer = gs_graphics_framebuffer_create(&fb_desc);
    gs_graphics_renderpass_desc_t rp_desc = gs_default_val();
    rp_desc.fbo = frame_buffer;
    rp_desc.color = &render_texture.texture;
    rp_desc.color_size = sizeof(render_texture.texture);
    gs_handle(gs_graphics_renderpass_t) render_pass = gs_graphics_renderpass_create(&rp_desc);

    card_entity_t card = {0};
    card.data = card_data;
    card.frame_buffer = frame_buffer;
    card.render_pass = render_pass;
    card.entity = (entity_t){0};
    card.entity.mesh = mesh_plane();
    card.entity.transform = gs_vqs_default();
    card.entity.transform.scale = CARD_SCALE;
    card.entity.transform.position = gs_v3(0, 0.1, 0);
    card.entity.material.texture = render_texture.texture;
    card.entity.material.color = gs_v4s(1);
    return card;
}

void card_entities_position_as_page(gs_camera_t *camera, card_entity_t *cards) {
    for (int i = 0; i >= gs_dyn_array_size(cards); i++) {
        gs_vqs_t transform = gs_vqs_default();
        transform.position.x = -4.25 + (i % 6) * 1.75;
        transform.position.y = 2 - (i / 6) * 2.25;
        cards[i].entity.transform = transform;
    }
}

void card_entities_position_as_hand(gs_camera_t *camera, card_entity_t *cards, bool bottom_of_screen) {
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
        gs_vqs target = cards[i].entity.transform;
        target.position.x = start_x + i * spacing;
        target.position.y = fabs(start_x + i * spacing) * fabs(start_x + i * spacing) * curve_amount * (1.f / spacing) + y_offset;
        target.position.z = 5; // units in front of camera (0, won't be seen)
        target.rotation = gs_quat_angle_axis(gs_deg2rad((start_tilt - i * fan_angle)), gs_v3(0, 1, 0)); // rotating on y
        transform_in_front_of_camera(camera, &cards[i].entity, target.position, target.rotation, 0.2);
    }
}

bool card_entities_contains_card(gs_dyn_array(card_entity_t) card_entities, card_data_t card) {
    for (int i = 0; i < gs_dyn_array_size(card_entities); i++) {
        if (card_entities[i].data.database_index == card.database_index) return true;
    }
    return false;
}

void card_entity_bake_texture(gs_immediate_draw_t *gsi, card_entity_t card_entity) {
    gsi_camera2D(gsi, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);
    gsi_texture(gsi, CARD_BG_TEXTURE); // set a source texture

    gs_command_buffer_t command_buffer = gs_command_buffer_new();
    gs_graphics_clear_action_t clear_action = gs_default_val();
    clear_action.flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH;
    clear_action.color[0] = 0.0f;
    clear_action.color[1] = 0.0f;
    clear_action.color[2] = 0.0f;
    clear_action.color[3] = 0.0f;
    gs_graphics_clear_desc_t clear = gs_default_val();
    clear.actions = &clear_action;
    clear.size = sizeof(clear_action);

    gs_graphics_renderpass_begin(&command_buffer, card_entity.render_pass);
    gs_graphics_set_viewport(&command_buffer, 0, 0, CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT);
    gs_graphics_clear(&command_buffer, &clear);

    gsi_rectvd(gsi,
               gs_v2(0.f, 0.f),
               gs_v2(CARD_TEXTURE_WIDTH, CARD_TEXTURE_HEIGHT),
               gs_v2(0.f, 0.f),
               gs_v2(1.f, 1.f),
               gs_color(50, card_entity.data.selectable ? 255 : 50, 50, 255),
               GS_GRAPHICS_PRIMITIVE_TRIANGLES);

    gsi_rectvd(gsi,
               gs_v2(12.f, 12.f),
               gs_v2(CARD_TEXTURE_WIDTH - 24, CARD_TEXTURE_HEIGHT - 24),
               gs_v2(0.f, 0.f),
               gs_v2(1.f, 1.f),
               GS_COLOR_WHITE,
               GS_GRAPHICS_PRIMITIVE_TRIANGLES);

    gs_vec2 text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, card_entity.data.name, -1); // -1 means null-terminated string

    gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, 24.f, card_entity.data.name,
             &CARD_FONT, false, card_entity.data.red ? 200 : 20, card_entity.data.green ? 200 : 20, card_entity.data.blue ? 200 : 20, 255);

    float ability_y_offset = CARD_TEXTURE_HEIGHT / 2.0f;
    float offset_increment = 100.f;
    if (card_entity.data.current_abilities.strike > 0) {
        char ability_buffer[20] = "Strike ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.strike);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.heal > 0) {
        char ability_buffer[20] = "Heal ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.heal);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.dull > 0) {
        char ability_buffer[20] = "Dull ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.dull);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.sharpen > 0) {
        char ability_buffer[20] = "Sharpen ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.sharpen);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.mass_strike > 0) {
        char ability_buffer[20] = "Mass Strike ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.mass_strike);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.mass_heal > 0) {
        char ability_buffer[20] = "Mass Heal ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.mass_heal);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.mass_dull > 0) {
        char ability_buffer[20] = "Mass Dull ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.mass_dull);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.mass_sharpen > 0) {
        char ability_buffer[20] = "Mass Sharpen ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "%d", card_entity.data.current_abilities.mass_sharpen);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.charge_health > 0) {
        char ability_buffer[20] = "Charge Health ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "+%d", card_entity.data.current_abilities.charge_health);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.charge_attack > 0) {
        char ability_buffer[20] = "Charge Attack ";
        size_t current_len = strlen(ability_buffer);
        snprintf(ability_buffer + current_len, sizeof(ability_buffer) - current_len, "+%d", card_entity.data.current_abilities.charge_attack);
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }

    if (card_entity.data.current_abilities.shield) {
        char ability_buffer[20] = "Shield";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.evade) {
        char ability_buffer[20] = "Evade";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.regenerate) {
        char ability_buffer[20] = "Regenerate";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.haste) {
        char ability_buffer[20] = "Haste";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.timebound) {
        char ability_buffer[20] = "Timebound";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.sacrifice) {
        char ability_buffer[20] = "Sacrifice";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.frozen) {
        char ability_buffer[20] = "Frozen";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.ward) {
        char ability_buffer[20] = "Ward";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.cancel) {
        char ability_buffer[20] = "Cancel";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }

    if (card_entity.data.current_abilities.bestow_shield) {
        char ability_buffer[20] = "Bestow Shield";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.bestow_regenerate) {
        char ability_buffer[20] = "Bestow Regenerate";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.bestow_haste) {
        char ability_buffer[20] = "Bestow Haste";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.bestow_timebound) {
        char ability_buffer[20] = "Bestow Timebound";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.bestow_sacrifice) {
        char ability_buffer[20] = "Bestow Sacrifice";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.bestow_frozen) {
        char ability_buffer[20] = "Bestow Frozen";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }
    if (card_entity.data.current_abilities.bestow_ward) {
        char ability_buffer[20] = "Bestow Ward";
        text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, ability_buffer, strlen(ability_buffer));
        gsi_text(gsi, CARD_TEXTURE_WIDTH * 0.5f - text_dimensions.x * 0.5f, ability_y_offset, ability_buffer, &CARD_FONT, false, 20, 20, 20, 255);
        ability_y_offset += offset_increment;
    }

    char attack_char_buffer[10];
    snprintf(attack_char_buffer, sizeof(attack_char_buffer), "%d", card_entity.data.current_attack);
    gsi_text(gsi, 50.f, CARD_TEXTURE_HEIGHT - 100, attack_char_buffer, &CARD_FONT, false, card_entity.data.current_attack < card_entity.data.attack ? 255 : 20, card_entity.data.current_attack > card_entity.data.attack ? 255 : 20, 20, 255);

    char health_char_buffer[10];
    snprintf(health_char_buffer, sizeof(health_char_buffer), "%d", card_entity.data.current_health);
    text_dimensions = gs_asset_font_text_dimensions(&CARD_FONT, health_char_buffer, strlen(health_char_buffer));
    gsi_text(gsi, CARD_TEXTURE_WIDTH - text_dimensions.x - 50.f, CARD_TEXTURE_HEIGHT - 100, health_char_buffer, &CARD_FONT, false, card_entity.data.current_health < card_entity.data.health ? 255 : 20, card_entity.data.current_health > card_entity.data.health ? 255 : 20, 20, 255);

    gsi_draw(gsi, &command_buffer);
    gs_graphics_renderpass_end(&command_buffer);
    gs_graphics_command_buffer_submit(&command_buffer);
}

#endif
