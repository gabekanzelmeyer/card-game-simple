#ifndef ENGINE_SHAPES_H
#define ENGINE_SHAPES_H

#include "gs.h"
#include "engine.h"

mesh_t mesh_plane() {
    vertex_t verts[4] = {
        {{-0.5f, 0.f, -0.5f}, {0.0, 1.0, 0.0}, {0.0, 1.0}},
        {{0.5f, 0.f, -0.5f}, {0.0, 1.0, 0.0}, {1.0, 1.0}},
        {{0.5f, 0.f,  0.5f}, {0.0, 1.0, 0.0}, {1.0, 0.0}},
        {{-0.5f, 0.f, 0.5f}, {0.0, 1.0, 0.0}, {0.0, 0.0}}
    };

    uint16_t indices[6] = {
        0, 2, 1,
        0, 3, 2
    };

    return mesh_create(verts, 4, indices, 6);
}

mesh_t mesh_cube() {
    vertex_t verts[24] = {
        // front
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        // back
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        // right
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        // left
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        // top
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        // bottom
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    uint16_t indices[36] = {
        0,  1,  2, 0,  2,  3, // front
        4,  5,  6, 4,  6,  7, //back
        8,  9, 10, 8, 10, 11, // right
        12, 13, 14, 12, 14, 15, // left
        16, 17, 18, 16, 18, 19, // top
        20, 21, 22, 20, 22, 23 // bottom
    };

    return mesh_create(verts, 24, indices, 36);
}

mesh_t mesh_sphere(float radius, int32_t stacks, int32_t slices) {
    gs_dyn_array(vertex_t) verts = NULL;
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

            vertex_t vert = {0};
            vert.position = gs_v3(x * radius, y * radius, z * radius);
            vert.normal = gs_vec3_norm(vert.position);
            vert.uv = gs_v2(u, v);
            gs_dyn_array_push(verts, vert);
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
            gs_dyn_array_push(indices, c);
            gs_dyn_array_push(indices, b);
            gs_dyn_array_push(indices, c);
            gs_dyn_array_push(indices, d);
            gs_dyn_array_push(indices, b);
        }
    }

    mesh_t m = mesh_create(verts, gs_dyn_array_size(verts), indices, gs_dyn_array_size(indices));

    gs_dyn_array_free(verts);
    gs_dyn_array_free(indices);
    return m;
}

mesh_t mesh_capsule(float radius, float cylinder_height,
                    int slices, int hemisphere_segments)
{
    int rings = hemisphere_segments * 2 + 1;
    int vertex_count = (rings + 1) * (slices + 1);
    int index_count = rings * slices * 6;

    vertex_t *verts = (vertex_t*)malloc(sizeof(vertex_t) * vertex_count);
    uint16_t *indices = (uint16_t*)malloc(sizeof(uint16_t) * index_count);

    if (!verts || !indices) {
        free(verts);
        free(indices);
        return (mesh_t) {0};
    }

    int v = 0;

    for (int ring = 0; ring <= rings; ring++) {
        float t = (float)ring / (float)rings;
        float theta = t * (float)M_PI;
        float sin_theta = sinf(theta);
        float cos_theta = cosf(theta);
        float y;
        float ring_radius;

        if (t < 0.5f) {
            y = cylinder_height * 0.5f + radius * cos_theta;
            ring_radius = radius * sin_theta;
        } else {
            y = -cylinder_height * 0.5f + radius * cos_theta;
            ring_radius = radius * sin_theta;
        }

        for (int slice = 0; slice <= slices; slice++) {
            float u = (float)slice / (float)slices;
            float phi = u * 2.0f * (float)M_PI;
            float cos_phi = cosf(phi);
            float sin_phi = sinf(phi);
            float x = ring_radius * cos_phi;
            float z = ring_radius * sin_phi;
            float normal_y = cos_theta;

            gs_vec3 normal = {
                sin_theta * cos_phi,
                normal_y,
                sin_theta * sin_phi
            };

            verts[v++] = (vertex_t) {
                .position = {x, y, z},
                .normal = normal,
                .uv = {u, 1.0f - t}
            };
        }
    }

    int i = 0;

    for (int ring = 0; ring < rings; ring++) {
        for (int slice = 0; slice < slices; slice++) {
            int a = ring * (slices + 1) + slice;
            int b = a + 1;
            int c = (ring + 1) * (slices + 1) + slice;
            int d = c + 1;

            indices[i++] = (uint16_t)a;
            indices[i++] = (uint16_t)b;
            indices[i++] = (uint16_t)c;
            indices[i++] = (uint16_t)b;
            indices[i++] = (uint16_t)d;
            indices[i++] = (uint16_t)c;
        }
    }

    mesh_t mesh = mesh_create(
        verts,
        vertex_count,
        indices,
        index_count
    );

    free(verts);
    free(indices);

    return mesh;
}

#endif
