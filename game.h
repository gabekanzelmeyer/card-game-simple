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

bool game_map_is_tile_empty(game_map_t *map, int x, int y) {
    return x >= 0 && x < map->width && y >= 0 && y < map->height && map->tiles[y * map->width + x] == NULL;
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

#endif
