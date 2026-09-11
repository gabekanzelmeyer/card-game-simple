#ifndef ENGINE_SHAPES_H
#define ENGINE_SHAPES_H

#include "gs.h"
#include "engine.h"

mesh_t mesh_plane() {
    gs_vec3 verts[4] = {
        gs_v3(-0.5f, 0.f, -0.5f),
        gs_v3( 0.5f, 0.f, -0.5f),
        gs_v3( 0.5f, 0.f,  0.5f),
        gs_v3(-0.5f, 0.f, 0.5f),
    };

    uint16_t indices[6] = {
        0, 2, 1,
        0, 3, 2
    };

    return mesh_create(verts, 4, indices, 6);
}

mesh_t mesh_sphere(float radius, int32_t stacks, int32_t slices) {
    gs_dyn_array(gs_vec3) verts = NULL;
    gs_dyn_array(uint16_t) indices = NULL;

    for (int32_t i = 0; i <= stacks; ++i)
    {
        float v = (float)i / (float)stacks;
        float phi = v * (float)GS_PI; // 0 .. pi
        for (int32_t j = 0; j <= slices; ++j)
        {
            float u = (float)j / (float)slices;
            float theta = u * 2.f * (float)GS_PI; // 0 .. 2pi
            float x = sinf(phi) * cosf(theta);
            float y = cosf(phi);
            float z = sinf(phi) * sinf(theta);
            gs_dyn_array_push(verts, gs_v3(x * radius, y * radius, z * radius));
        }
    }

    int32_t verts_per_row = slices + 1;
    for (int32_t i = 0; i < stacks; ++i)
    {
        for (int32_t j = 0; j < slices; ++j)
        {
            uint16_t a = (uint16_t)(i * verts_per_row + j);
            uint16_t b = (uint16_t)(a + verts_per_row);
            uint16_t c = (uint16_t)(a + 1);
            uint16_t d = (uint16_t)(b + 1);
            gs_dyn_array_push(indices, a);
            gs_dyn_array_push(indices, b);
            gs_dyn_array_push(indices, c);
            gs_dyn_array_push(indices, c);
            gs_dyn_array_push(indices, b);
            gs_dyn_array_push(indices, d);
        }
    }

    mesh_t m = mesh_create(verts, gs_dyn_array_size(verts), indices, gs_dyn_array_size(indices));

    gs_dyn_array_free(verts);
    gs_dyn_array_free(indices);
    return m;
}

#endif
