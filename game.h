#ifndef GAME_H
#define GAME_H

#include<stdlib.h>

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "engine.h"

// macro that allows for erasing and element from a gs_dyn_array and keeping the order
#define gs_dyn_array_erase(__ARR, __IDX)\
do {\
    if ((__ARR) && (uint32_t)(__IDX) < gs_dyn_array_size(__ARR)) {\
        memmove(\
        &(__ARR)[__IDX],\
        &(__ARR)[(__IDX) + 1],\
        (gs_dyn_array_size(__ARR) - (__IDX) - 1) * sizeof(*(__ARR))\
        );\
        gs_dyn_array_head(__ARR)->size--;\
    }\
} while (0)

enum game_mode {
    MENU,
    LIBRARY,
    CARD_GAME,
    SIM_CARD_GAME,
    WORLD
};

typedef struct {
    uint32_t width;
    uint32_t height;
    entity_t **tiles;
} game_map_t;

typedef struct {

} game_interaction_step_t;

game_map_t game_map_create(uint32_t width, uint32_t height) {
    game_map_t map = {0};
    map.width = width;
    map.height = height;
    map.tiles = (entity_t **)malloc(sizeof(entity_t*) * width * height);
    for (int i = 0; i < width * height; i++) {
        map.tiles[i] = NULL;
    }
    return map;
}

void game_map_free(game_map_t *map) {
    free(map->tiles);
}

bool game_map_is_tile_empty(game_map_t *map, int x, int z) {
    return x >= 0 && x < map->width && z >= 0 && z < map->height && map->tiles[z * map->width + x] == NULL;
}

void game_map_add(game_map_t *map, entity_t *entity, int x, int y) {
    int index = y * map->width + x;
    if (map->tiles[index] == NULL) {
        entity->transform.position.x = x;
        entity->transform.position.y = 0.5;
        entity->transform.position.z = y;
        map->tiles[index] = entity;
    }
}

void game_map_move(game_map_t *map, entity_t *entity, gs_vec3 dir, float dt) {
    int prev_x = lroundf(entity->prev.position.x);
    int prev_z = lroundf(entity->prev.position.z);
    int target_x = prev_x + lroundf(dir.x);
    int target_z = prev_z + lroundf(dir.z);
    int next_x = prev_x;
    int next_z = prev_z;

    if (gs_vec3_len(dir) > 0.f && (entity->lerp == 0 || entity->lerp >= 1)) {
        gs_vec3 normalized_dir = gs_vec3_norm(dir);
        gs_vec3 local_forward = gs_v3(0.f, 0.f, -1.f);
        gs_quat target_rotation = gs_quat_from_to_rotation(local_forward, normalized_dir);
        entity->transform.rotation = target_rotation;
        entity->prev.rotation = target_rotation;
        entity->next.rotation = target_rotation;

        if (game_map_is_tile_empty(map, target_x, target_z)) {
            next_x = target_x;
            next_z = target_z;
        }
        // NOT sure I really want this "avoidance" logic
        /*else if (move.x != 0 && move.z == 0 && game_map_is_tile_empty(&map, target_x, target_z + 1) && game_map_is_tile_empty(&map, prev_x, target_z + 1)) {
         *       next_x = target_x;
         *       next_z = target_z + 1;
        } else if (move.x != 0 && move.z == 0 && game_map_is_tile_empty(&map, target_x, target_z - 1) && game_map_is_tile_empty(&map, prev_x, target_z - 1)) {
            next_x = target_x;
            next_z = target_z - 1;
        } else if (move.x == 0 && move.z != 0 && game_map_is_tile_empty(&map, target_x + 1, target_z) && game_map_is_tile_empty(&map, target_x + 1, prev_z)) {
            next_x = target_x + 1;
            next_z = target_z;
        } else if (move.x == 0 && move.z != 0 && game_map_is_tile_empty(&map, target_x - 1, target_z) && game_map_is_tile_empty(&map, target_x - 1, prev_z)) {
            next_x = target_x - 1;
            next_z = target_z;
        } else if (game_map_is_tile_empty(&map, prev_x, target_z)) {
            next_x = prev_x;
            next_z = target_z;
        } else if (game_map_is_tile_empty(&map, target_x, prev_z)) {
            next_x = target_x;
            next_z = prev_z;
        }*/
    }

    if (next_x != prev_x || next_z != prev_z) {
        map->tiles[prev_z * map->width + prev_x] = NULL;
        map->tiles[next_z * map->width + next_x] = entity;
        dir = gs_v3(next_x - prev_x, 0, next_z - prev_z);
        gs_vec3 normalized_dir = gs_vec3_norm(dir);
        gs_vec3 local_forward = gs_v3(0.f, 0.f, -1.f);
        gs_quat target_rotation = gs_quat_from_to_rotation(local_forward, normalized_dir);
        entity->transform.rotation = target_rotation;
        entity->prev.rotation = target_rotation;
        entity->next.rotation = target_rotation;
    } else {
        dir = gs_v3s(0);
    }

    if (gs_vec3_len(gs_vec3_sub(entity->next.position, entity->prev.position)) > 0)  {
        entity->lerp += dt / 0.2f;
    }
    if (gs_vec3_len(dir) > 0.f && entity->lerp >= 1.0f) {
        entity->prev.position.x = lroundf(entity->next.position.x);
        entity->prev.position.z = lroundf(entity->next.position.z);
        entity->next.position = gs_vec3_add(entity->next.position, dir);
        entity->lerp -= 1.0;
    } else if (gs_vec3_len(dir) > 0.f && entity->lerp == 0.0f) {
        entity->prev.position.x = lroundf(entity->next.position.x);
        entity->prev.position.z = lroundf(entity->next.position.z);
        entity->next.position = gs_vec3_add(entity->next.position, dir);
        entity->lerp = dt / 0.2f;
    } else if (entity->lerp >= 1.0f) {
        entity->prev.position.x = lroundf(entity->next.position.x);
        entity->prev.position.z = lroundf(entity->next.position.z);
        entity->lerp = 0;
    }
    lerp_transform_linear(&entity->transform, &entity->prev, &entity->next, entity->lerp);
}

#endif
