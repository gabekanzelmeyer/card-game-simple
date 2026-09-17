#ifndef GAME_H
#define GAME_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

// #define CGLTF_IMPLEMENTATION
// #include "external/cgltf/cgltf.h"

// #include "card_renderer.h"

// macro that allows for erasing and element from a gs_dyn_array and keeping the order
// used to remove cards from player / opponent hands
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

#endif
