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
    gs_handle(gs_graphics_texture_t) texture;
    gs_vec4_t color;
} material_t;

typedef struct entity_t {
    mesh_t mesh;
    gs_vqs_t transform;
    gs_vqs_t prev;
    gs_vqs_t next;
    float lerp;
    float lerp_duration;
    material_t material;
    struct gs_dyn_array(entity_t) children;
} entity_t;

typedef struct {
    gs_handle(gs_graphics_texture_t) texture;
    uint32_t width;
    uint32_t height;
} render_texture_t;

typedef struct {
    gs_command_buffer_t cb;
    gs_immediate_draw_t gsi;
    gs_camera_t camera;
} engine_t;

gs_handle(gs_graphics_texture_t) NO_TEXTURE;

engine_t engine_init() {
    engine_t engine = (engine_t){0};
    engine.cb = gs_command_buffer_new();
    engine.gsi = gs_immediate_draw_new();

    engine.camera = gs_camera_perspective();
    engine.camera.fov = 60.f;
    engine.camera.near_plane = 0.1f;
    engine.camera.far_plane = 1000.f;
    // engine.camera = gs_camera_default();
    // engine.camera.proj_type = GS_PROJECTION_TYPE_ORTHOGRAPHIC;
    // engine.camera.ortho_scale = 10.f;

    // create a simple 1x1 white texture to be used for NO_TEXTURE
    uint8_t white[4] = {255, 255, 255, 255};
    gs_graphics_texture_desc_t no_texture_desc = gs_default_val();
    no_texture_desc.width = 1;
    no_texture_desc.height = 1;
    no_texture_desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
    no_texture_desc.data[0] = white;
    no_texture_desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    no_texture_desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    NO_TEXTURE = gs_graphics_texture_create(&no_texture_desc);

    return engine;
}

render_texture_t render_texture_create(uint32_t width, uint32_t height) {
    gs_graphics_texture_desc_t desc = gs_default_val();
    desc.width = width;
    desc.height = height;
    desc.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8;
    desc.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;
    desc.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR;

    render_texture_t rt = {0};
    rt.texture = gs_graphics_texture_create(&desc);
    rt.width = width;
    rt.height = height;
    return rt;
}

mesh_t mesh_create(vertex_t* verts, uint32_t vert_count, uint16_t* indices, uint32_t index_count) {
    mesh_t m = {0};

    gs_graphics_vertex_buffer_desc_t vdesc = gs_default_val();
    vdesc.data = verts;
    vdesc.size = vert_count * sizeof(vertex_t);
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
    char *buffer = NULL;
    long length = 0;
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
        buffer[length] = '\0';
    }

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
    gs_graphics_vertex_attribute_desc_t attrs[3] = gs_default_val();
    attrs[0].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3;
    memcpy(attrs[0].name, "a_position", sizeof("a_position"));
    attrs[1].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3;
    memcpy(attrs[1].name, "a_normal", sizeof("a_normal"));
    attrs[2].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2;
    memcpy(attrs[2].name, "a_uv", sizeof("a_uv"));

    gs_graphics_pipeline_desc_t pdesc = gs_default_val();
    pdesc.raster.shader = shader;
    pdesc.raster.index_buffer_element_size = sizeof(uint16_t);
    pdesc.raster.face_culling = GS_GRAPHICS_FACE_CULLING_BACK;
    pdesc.raster.primitive = GS_GRAPHICS_PRIMITIVE_TRIANGLES;
    pdesc.depth.func = GS_GRAPHICS_DEPTH_FUNC_LESS;
    pdesc.blend.func = GS_GRAPHICS_BLEND_EQUATION_ADD;
    pdesc.blend.src = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA;
    pdesc.blend.dst = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA;
    pdesc.layout.attrs = attrs;
    pdesc.layout.size = sizeof(attrs);

    return gs_graphics_pipeline_create(&pdesc);
}

shader_t shader_standard() {
    shader_t shader = {0};
    shader.shader = shader_create("shaders/standard.vert", "shaders/standard.frag");
    shader.pipeline = pipeline_create(shader.shader);
    shader.u_mvp = uniform_create("u_mvp", GS_GRAPHICS_UNIFORM_MAT4, GS_GRAPHICS_SHADER_STAGE_VERTEX);
    shader.u_color = uniform_create("u_color", GS_GRAPHICS_UNIFORM_VEC4, GS_GRAPHICS_SHADER_STAGE_FRAGMENT);
    shader.u_texture = uniform_create("u_texture", GS_GRAPHICS_UNIFORM_SAMPLER2D, GS_GRAPHICS_SHADER_STAGE_FRAGMENT);
    return shader;
}

void transform_lerp(gs_vqs *current, gs_vqs *prev, gs_vqs *next, float lerp) {
    current->position.x = gs_interp_smoothstep(prev->position.x, next->position.x, lerp);
    current->position.y = gs_interp_smoothstep(prev->position.y, next->position.y, lerp);
    current->position.z = gs_interp_smoothstep(prev->position.z, next->position.z, lerp);

    current->scale.x = gs_interp_smoothstep(prev->scale.x, next->scale.x, lerp);
    current->scale.y = gs_interp_smoothstep(prev->scale.y, next->scale.y, lerp);
    current->scale.z = gs_interp_smoothstep(prev->scale.z, next->scale.z, lerp);

    current->rotation.x = gs_interp_smoothstep(prev->rotation.x, next->rotation.x, lerp);
    current->rotation.y = gs_interp_smoothstep(prev->rotation.y, next->rotation.y, lerp);
    current->rotation.z = gs_interp_smoothstep(prev->rotation.z, next->rotation.z, lerp);
    current->rotation.w = gs_interp_smoothstep(prev->rotation.w, next->rotation.w, lerp);
}

void entity_animate(entity_t *entity, float dt) {
    entity->lerp += dt / entity->lerp_duration;
    if (entity->lerp >= 1) {
        entity->lerp = 1;
    }
    transform_lerp(&entity->transform, &entity->prev, &entity->next, entity->lerp);
}

void entities_animate(gs_dyn_array(entity_t) entities, float dt) {
    for (int i = 0; i < gs_dyn_array_size(entities); i++) {
        entity_animate(&entities[i], dt);
    }
}

void entity_animation_start(entity_t *entity, gs_vqs target, float duration) {
    entity->prev = entity->transform;
    entity->next = target;
    entity->lerp_duration = duration;
    entity->lerp = 0.0f;
}

void draw_entity(entity_t *entity, gs_mat4 view_projection, shader_t *shader, engine_t *engine) {
    gs_mat4 model = gs_vqs_to_mat4(&entity->transform);
    gs_mat4 mvp = gs_mat4_mul(view_projection, model);

    gs_graphics_bind_vertex_buffer_desc_t vb = gs_default_val();
    vb.buffer = entity->mesh.vbo;

    gs_graphics_bind_index_buffer_desc_t ib = gs_default_val();
    ib.buffer = entity->mesh.ibo;

    gs_graphics_bind_uniform_desc_t uniforms[3] = gs_default_val();
    uniforms[0].uniform = shader->u_mvp;
    uniforms[0].data = &mvp;
    uniforms[1].uniform = shader->u_color;
    uniforms[1].data = &entity->material.color;
    uniforms[2].uniform = shader->u_texture;
    uniforms[2].data = &entity->material.texture;

    gs_graphics_bind_desc_t binds = gs_default_val();
    binds.vertex_buffers.desc = &vb;
    binds.vertex_buffers.size = sizeof(vb);
    binds.index_buffers.desc = &ib;
    binds.index_buffers.size = sizeof(ib);
    binds.uniforms.desc = uniforms;
    binds.uniforms.size = sizeof(uniforms);

    gs_graphics_apply_bindings(&engine->cb, &binds);

    gs_graphics_draw_desc_t draw = gs_default_val();
    draw.start = 0;
    draw.count = entity->mesh.index_count;
    gs_graphics_draw(&engine->cb, &draw);
}

#endif
