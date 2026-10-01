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
    WORLD,
    EDITOR
};

typedef struct {
    uint32_t width;
    uint32_t height;
    gs_dyn_array(float) height_map;
    entity_t **tiles;
} game_map_t;

typedef struct game_interaction_t {
    const char *text;
    const char *option1;
    const char *option2;
    gs_vqs *transform;
    gs_vec2 offset;
    struct game_interaction_t * option1_next;
    struct game_interaction_t * option2_next;
} game_interaction_t;

game_map_t game_map_create(uint32_t width, uint32_t height, float* height_map) {
    game_map_t map = {0};
    map.width = width;
    map.height = height;
    map.height_map = NULL;
    map.tiles = (entity_t **)malloc(sizeof(entity_t*) * width * height);
    for (int i = 0; i < width * height; i++) {
        map.tiles[i] = NULL;
    }
    for (int i = 0; i < width * height; i++) {
        gs_dyn_array_push(map.height_map, height_map == NULL ? 0 : height_map[i]);
    }
    return map;
}

void game_map_free(game_map_t *map) {
    free(map->tiles);
}

void game_map_save(game_map_t *map, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("ERROR opening map file for writing");
        exit(1);
    }
    fprintf(f, "%i\n", map->width);
    fprintf(f, "%i\n", map->height);
    for (int i = 0; i < map->width * map->height; i++) {
        fprintf(f, "%f\n", map->height_map[i]);
    }

    fclose(f);
}

game_map_t game_map_load(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("ERROR opening map file for reading");
        exit(1);
    }

    int width;
    int height;
    fscanf(f, "%i", &width);
    fscanf(f, "%i", &height);

    float *heights = (float*)malloc(width * height * sizeof(float));

    float temp;
    int count = 0;
    while (fscanf(f, "%f", &temp) == 1) {
        heights[count++] = temp;
    }

    game_map_t map = game_map_create(width, height, heights);

    free(heights);
    fclose(f);
    return map;
}

bool game_map_is_tile_empty(game_map_t *map, int x, int z) {
    return x >= 0 && x < map->width && z >= 0 && z < map->height && map->tiles[z * map->width + x] == NULL;
}

void game_map_add(game_map_t *map, entity_t *entity, int x, int y) {
    int index = y * map->width + x;
    if (map->tiles[index] == NULL) {
        entity->transform.position.x = x;
        entity->transform.position.y = map->height_map[y * (map->width + 1) + x] + 0.5;
        entity->transform.position.z = y;
        entity->prev = entity->transform;
        entity->next = entity->transform;
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
    } else {
        dir = gs_v3s(0);
    }

    float speed = 5.0f;
    int height_index = next_z * map->width + lroundf(next_x);

    if (gs_vec3_len(gs_vec3_sub(entity->next.position, entity->prev.position)) > 0)  {
        entity->lerp += dt * speed;
    }
    if (gs_vec3_len(dir) > 0.f && entity->lerp >= 1.0f) {
        entity->prev.position = entity->next.position;
        entity->next.position.x = next_x;
        entity->next.position.y = map->height_map[height_index] + 0.5;
        entity->next.position.z = next_z;
        entity->lerp -= 1.0;
    } else if (gs_vec3_len(dir) > 0.f && entity->lerp == 0.0f) {
        entity->prev.position = entity->next.position;
        entity->next.position.x = next_x;
        entity->next.position.y = map->height_map[height_index] + 0.5;
        entity->next.position.z = next_z;
        entity->lerp = dt * speed;
    } else if (entity->lerp >= 1.0f) {
        entity->prev.position = entity->next.position;
        entity->lerp = 0.f;
    }
    lerp_transform_linear(&entity->transform, &entity->prev, &entity->next, entity->lerp);
}

bool game_map_terrain_intersect(game_map_t *map, ray_t r, float max_dist, gs_vec3* out) {
    const float step = 0.5f;
    float t_prev = 0.0f;

    for (float t = 0.0f; t < max_dist; t += step) {
        gs_vec3 p = gs_vec3_add(r.origin, gs_vec3_scale(r.dir, t));
        if (p.x < 0 || p.z < 0 || p.x >= map->width || p.y >= map->height) continue;

        if (p.y <= map->height_map[lroundf(p.z) * map->width + lroundf(p.x)]) {
            // Crossed the surface: binary search between t_prev and t
            float lo = t_prev, hi = t;
            for (int i = 0; i < 8; i++) {
                float mid = (lo + hi) * 0.5f;
                gs_vec3 m = gs_vec3_add(r.origin, gs_vec3_scale(r.dir, mid));
                if (m.y <= map->height_map[lroundf(m.z) * map->width + lroundf(m.x)]) {
                    hi = mid;
                } else {
                    lo = mid;
                }
            }
            *out = gs_vec3_add(r.origin, gs_vec3_scale(r.dir, hi));
            return true;
        }
        t_prev = t;
    }
    return false;
}

#endif
