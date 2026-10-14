#ifndef GAME_H
#define GAME_H

#include<stdlib.h>

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

#include "engine.h"
#include "engine_shapes.h"

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
    const char *name;
    uint16_t type;
    uint16_t width;
    uint16_t depth;
    uint32_t x;
    uint32_t z;
    entity_t *entity;
} game_map_object_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    float *height_map;
    game_map_object_t *objects;
    game_map_object_t **extents;
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
    map.height_map = (float*)malloc(sizeof(float) * width * height);
    map.objects = (game_map_object_t*)malloc(sizeof(game_map_object_t) * width * height);
    map.extents = (game_map_object_t**)malloc(sizeof(game_map_object_t*) * width * height);
    for (int i = 0; i < width * height; i++) {
        map.height_map[i] = height_map == NULL ? 0 : height_map[i];
        map.objects[i] = (game_map_object_t){0};
        map.extents[i] = NULL;
    }
    return map;
}

void game_map_free(game_map_t *map) {
    free(map->height_map);
    free(map->objects);
    free(map->extents);
}

void game_map_add(game_map_t *map, game_map_object_t object, int x, int z) {
    int index = z * map->width + x;
    if (map->extents[index] == NULL) {
        object.x = x;
        object.z = z;
        object.entity->transform.position.x = x;
        object.entity->transform.position.y = map->height_map[z * (map->width + 1) + x] + 0.5;
        object.entity->transform.position.z = z;
        object.entity->prev = object.entity->transform;
        object.entity->next = object.entity->transform;
        map->objects[index] = object;

        for (int i = x - object.width; i < x + object.width + 1; i++) {
            for (int j = z - object.depth; j < z + object.depth + 1; j++) {
                int extent_index = j * map->width + i;
                map->extents[extent_index] = &map->objects[index];
            }
        }
    }
}

void game_map_remove(game_map_t *map, int x, int z) {
    int index = z * map->width + x;
    if (map->extents[index] != NULL) {
        game_map_object_t object = *map->extents[index];

        entity_free(object.entity);

        int object_index = object.z * map->width + object.x;
        map->objects[object_index] = (game_map_object_t){0};
        // remove extents
        for (int i = object.x - object.width; i < object.x + object.width + 1; i++) {
            for (int j = object.z - object.depth; j < object.z + object.depth + 1; j++) {
                int extent_index = j * map->width + i;
                map->extents[extent_index] = NULL;
            }
        }
    }
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
    int object_count = 0;
    for (int i = 0; i < map->width * map->height; i++) {
        if (map->objects[i].name != NULL) {
            object_count++;
        }
    }
    fprintf(f, "%i\n", object_count);
    for (int i = 0; i < map->width * map->height; i++) {
        if (map->objects[i].name != NULL) {
            fprintf(f, "%i\n", map->objects[i].x);
            fprintf(f, "%i\n", map->objects[i].z);
            fprintf(f, "%s\n", map->objects[i].name);
            fprintf(f, "%i\n", map->objects[i].type);
            fprintf(f, "%i\n", map->objects[i].width);
            fprintf(f, "%i\n", map->objects[i].depth);
        }
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
    for (int i = 0; i < width * height; i++) {
        fscanf(f, "%f", &temp);
        heights[i] = temp;
    }

    game_map_t map = game_map_create(width, height, heights);

    int object_count;
    fscanf(f, "%i", &object_count);
    for (int i = 0; i < object_count; i++) {
        int pos_x, pos_z, type, width, depth;
        char name[50];
        fscanf(f, "%i", &pos_x);
        fscanf(f, "%i", &pos_z);
        fscanf(f, "%s", name);
        fscanf(f, "%i", &type);
        fscanf(f, "%i", &width);
        fscanf(f, "%i", &depth);

        game_map_object_t object = {0};
        object.width = width;
        object.depth = depth;
        object.name = name;
        object.entity = (entity_t*)malloc(sizeof(entity_t));
        // TODO: based on type make a different entity, just assume building for now
        *object.entity = entity_create();
        object.entity->mesh = mesh_cube();
        object.entity->transform.scale = gs_v3(1 + width * 2, 1, 1 + depth * 2);
        object.entity->material.color = gs_v4(0.8, 0.3, 0.8, 1.0);
        game_map_add(&map, object, pos_x, pos_z);
    }

    free(heights);
    fclose(f);
    return map;
}

bool game_map_is_tile_empty(game_map_t *map, int x, int z) {
    return x >= 0 && x < map->width && z >= 0 && z < map->height && map->extents[z * map->width + x] == NULL;
}

void game_map_move(game_map_t *map, game_map_object_t object, gs_vec3 dir, float dt) {
    int prev_x = lroundf(object.entity->prev.position.x);
    int prev_z = lroundf(object.entity->prev.position.z);
    int target_x = prev_x + lroundf(dir.x);
    int target_z = prev_z + lroundf(dir.z);
    int next_x = prev_x;
    int next_z = prev_z;

    if (gs_vec3_len(dir) > 0.f && (object.entity->lerp == 0 || object.entity->lerp >= 1)) {
        gs_vec3 normalized_dir = gs_vec3_norm(dir);
        gs_vec3 local_forward = gs_v3(0.f, 0.f, -1.f);
        gs_quat target_rotation = gs_quat_from_to_rotation(local_forward, normalized_dir);
        object.entity->transform.rotation = target_rotation;
        object.entity->prev.rotation = target_rotation;
        object.entity->next.rotation = target_rotation;

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
        object.x = next_x;
        object.z = next_z;
        map->objects[prev_z * map->width + prev_x] = (game_map_object_t){0};
        map->objects[next_z * map->width + next_x] = object;

        for (int i = prev_x - object.width; i < prev_x + object.width + 1; i++) {
            for (int j = prev_z - object.depth; j < prev_z + object.depth + 1; j++) {
                int empty_index = j * map->width + i;
                map->extents[empty_index] = NULL;
            }
        }
        for (int i = next_x - object.width; i < next_x + object.width + 1; i++) {
            for (int j = next_z - object.depth; j < next_z + object.depth + 1; j++) {
                int empty_index = j * map->width + i;
                map->extents[empty_index] = &map->objects[next_z * map->width + next_x];
            }
        }
        dir = gs_v3(next_x - prev_x, 0, next_z - prev_z);
    } else {
        dir = gs_v3s(0);
    }

    float speed = 5.0f;
    int height_index = next_z * map->width + lroundf(next_x);

    if (gs_vec3_len(gs_vec3_sub(object.entity->next.position, object.entity->prev.position)) > 0)  {
        object.entity->lerp += dt * speed;
    }
    if (gs_vec3_len(dir) > 0.f && object.entity->lerp >= 1.0f) {
        object.entity->prev.position = object.entity->next.position;
        object.entity->next.position.x = next_x;
        object.entity->next.position.y = map->height_map[height_index] + 0.5;
        object.entity->next.position.z = next_z;
        object.entity->lerp -= 1.0;
    } else if (gs_vec3_len(dir) > 0.f && object.entity->lerp == 0.0f) {
        object.entity->prev.position = object.entity->next.position;
        object.entity->next.position.x = next_x;
        object.entity->next.position.y = map->height_map[height_index] + 0.5;
        object.entity->next.position.z = next_z;
        object.entity->lerp = dt * speed;
    } else if (object.entity->lerp >= 1.0f) {
        object.entity->prev.position = object.entity->next.position;
        object.entity->lerp = 0.f;
    }
    lerp_transform_linear(&object.entity->transform, &object.entity->prev, &object.entity->next, object.entity->lerp);
}

bool game_map_terrain_intersect(game_map_t *map, ray_t r, float max_dist, gs_vec3* out) {
    const float step = 0.5f;
    float t_prev = 0.0f;

    for (float t = 0.0f; t < max_dist; t += step) {
        gs_vec3 p = gs_vec3_add(r.origin, gs_vec3_scale(r.dir, t));
        p.x = lroundf(p.x);
        p.z = lroundf(p.z);
        if (p.x < 0 || p.z < 0 || p.x >= map->width || p.z >= map->height) continue;

        if (p.y <= map->height_map[(int)p.z * map->width + (int)p.x]) {
            // Crossed the surface: binary search between t_prev and t
            float lo = t_prev;
            float hi = t;
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
