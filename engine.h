#ifndef ENGINE_H
#define ENGINE_H

#include "gs.h"
#include "util/gs_idraw.h"
#include "util/gs_gui.h"

typedef struct {
    gs_vec3 position;
    gs_vec3 normal;
    gs_vec2 uv;
} vertex_t;

typedef struct {
    gs_handle(gs_graphics_vertex_buffer_t) vbo;
    gs_handle(gs_graphics_index_buffer_t) ibo;
    uint32_t index_count;
} mesh_t;

typedef struct {
    gs_handle(gs_graphics_shader_t) shader;
    gs_handle(gs_graphics_pipeline_t) pipeline;
    gs_handle(gs_graphics_uniform_t) u_mvp;
    gs_handle(gs_graphics_uniform_t) u_color;
    gs_handle(gs_graphics_uniform_t) u_texture;
} shader_t;

typedef struct {
    gs_command_buffer_t cb;
    gs_camera_t camera;
    gs_dyn_array(mesh_t) meshes;
} engine_t;

mesh_t mesh_create(gs_vec3* verts, uint32_t vert_count, uint16_t* indices, uint32_t index_count) {
    mesh_t m = {0};

    gs_graphics_vertex_buffer_desc_t vdesc = gs_default_val();
    vdesc.data = verts;
    vdesc.size = vert_count * sizeof(gs_vec3);
    vdesc.usage = GS_GRAPHICS_BUFFER_USAGE_STATIC;
    m.vbo = gs_graphics_vertex_buffer_create(&vdesc);

    gs_graphics_index_buffer_desc_t idesc = gs_default_val();
    idesc.data = indices;
    idesc.size = index_count * sizeof(uint16_t);
    idesc.usage = GS_GRAPHICS_BUFFER_USAGE_STATIC;
    m.ibo = gs_graphics_index_buffer_create(&idesc);

    m.index_count = index_count;
    return m;
}

char* load_file(const char* filename) {
    char *buffer = 0;
    long length;
    FILE *f = fopen(filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = (char*)malloc(length + 1);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }
    buffer[length] = '\0';
    return buffer;
}

static gs_handle(gs_graphics_shader_t) shader_create(const char *vs_file, const char *fs_file) {
    const char* vsrc = load_file(vs_file);
    const char* fsrc = load_file(fs_file);

    gs_graphics_shader_source_desc_t sources[2] = gs_default_val();
    sources[0].type = GS_GRAPHICS_SHADER_STAGE_VERTEX;
    sources[0].source = vsrc;
    sources[1].type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT;
    sources[1].source = fsrc;

    gs_graphics_shader_desc_t sdesc = gs_default_val();
    sdesc.sources = sources;
    sdesc.size = sizeof(sources);
    memcpy(sdesc.name, "standard_shader", sizeof("standard_shader"));

    return gs_graphics_shader_create(&sdesc);
}

static gs_handle(gs_graphics_uniform_t) uniform_create(const char* name, gs_graphics_uniform_type type, gs_graphics_shader_stage_type stage) {
    gs_graphics_uniform_layout_desc_t layout = gs_default_val();
    layout.type = type;

    gs_graphics_uniform_desc_t desc = gs_default_val();
    desc.stage = stage;
    desc.layout = &layout;
    desc.layout_size = sizeof(layout);
    memcpy(desc.name, name, strlen(name) + 1);

    return gs_graphics_uniform_create(&desc);
}

static gs_handle(gs_graphics_pipeline_t) pipeline_create(gs_handle(gs_graphics_shader_t) shader) {
    gs_graphics_vertex_attribute_desc_t attrs[1] = gs_default_val();
    attrs[0].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3;
    memcpy(attrs[0].name, "a_position", sizeof("a_position"));
    // attrs[1].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3;
    // memcpy(attrs[1].name, "a_normal", sizeof("a_normal"));
    // attrs[2].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2;
    // memcpy(attrs[2].name, "a_uv", sizeof("a_uv"));

    gs_graphics_pipeline_desc_t pdesc = gs_default_val();
    pdesc.raster.shader = shader;
    pdesc.raster.index_buffer_element_size = sizeof(uint16_t);
    pdesc.raster.face_culling = GS_GRAPHICS_FACE_CULLING_BACK;
    pdesc.raster.primitive = GS_GRAPHICS_PRIMITIVE_TRIANGLES;
    pdesc.depth.func = GS_GRAPHICS_DEPTH_FUNC_LESS;
    pdesc.layout.attrs = attrs;
    pdesc.layout.size = sizeof(attrs);

    return gs_graphics_pipeline_create(&pdesc);
}

shader_t shader_standard() {
    shader_t shader = {0};
    shader.shader = shader_create("shaders/standard.vert", "shaders/standard.frag");
    shader.u_mvp = uniform_create("u_mvp", GS_GRAPHICS_UNIFORM_MAT4, GS_GRAPHICS_SHADER_STAGE_VERTEX);
    shader.u_color = uniform_create("u_color", GS_GRAPHICS_UNIFORM_VEC4, GS_GRAPHICS_SHADER_STAGE_FRAGMENT);
    shader.u_texture = uniform_create("u_texture", GS_GRAPHICS_UNIFORM_SAMPLER2D, GS_GRAPHICS_SHADER_STAGE_FRAGMENT);
    shader.pipeline = pipeline_create(shader.shader);
    return shader;
}

void draw_mesh(mesh_t* mesh, gs_mat4 model, gs_mat4 view_projection, gs_vec4 color, shader_t shader, engine_t engine) {
    gs_mat4 mvp = gs_mat4_mul(view_projection, model);

    gs_graphics_bind_vertex_buffer_desc_t vb = gs_default_val();
    vb.buffer = mesh->vbo;

    gs_graphics_bind_index_buffer_desc_t ib = gs_default_val();
    ib.buffer = mesh->ibo;

    gs_graphics_bind_uniform_desc_t uniforms[2] = gs_default_val();
    uniforms[0].uniform = shader.u_mvp;
    uniforms[0].data = &mvp;
    uniforms[1].uniform = shader.u_color;
    uniforms[1].data = &color;
    // uniforms[2].uniform = shader.u_texture;
    // uniforms[2].data = &color;

    gs_graphics_bind_desc_t binds = gs_default_val();
    binds.vertex_buffers.desc = &vb;
    binds.vertex_buffers.size = sizeof(vb);
    binds.index_buffers.desc = &ib;
    binds.index_buffers.size = sizeof(ib);
    binds.uniforms.desc = uniforms;
    binds.uniforms.size = sizeof(uniforms);

    gs_graphics_apply_bindings(&engine.cb, &binds);

    gs_graphics_draw_desc_t draw = gs_default_val();
    draw.start = 0;
    draw.count = mesh->index_count;
    gs_graphics_draw(&engine.cb, &draw);
}

#endif
