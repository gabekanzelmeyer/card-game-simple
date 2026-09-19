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
    gs_gui_context_t gui;
    gs_camera_t camera;
    shader_t standard_shader;
    gs_asset_font_t standard_font;
} engine_t;

gs_handle(gs_graphics_texture_t) NO_TEXTURE;

static gs_handle(gs_graphics_shader_t) shader_create(const char *vs_file, const char *fs_file);
static gs_handle(gs_graphics_uniform_t) uniform_create(const char* name, gs_graphics_uniform_type type, gs_graphics_shader_stage_type stage);
static gs_handle(gs_graphics_pipeline_t) pipeline_create(gs_handle(gs_graphics_shader_t) shader);
void lerp_entity_init(entity_t *entity, gs_vqs target, float duration);

void engine_init(engine_t *engine) {
    *engine = (engine_t){0};
    engine->cb = gs_command_buffer_new();
    engine->gsi = gs_immediate_draw_new();
    gs_gui_init(&engine->gui, gs_platform_main_window());

    engine->camera = gs_camera_perspective();
    engine->camera.fov = 60.f;
    engine->camera.near_plane = 0.1f;
    engine->camera.far_plane = 1000.f;
    // engine->camera = gs_camera_default();
    // engine->camera.proj_type = GS_PROJECTION_TYPE_ORTHOGRAPHIC;
    // engine->camera.ortho_scale = 10.f;

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

    engine->standard_shader = (shader_t){0};
    engine->standard_shader.shader = shader_create("shaders/standard.vert", "shaders/standard.frag");
    engine->standard_shader.pipeline = pipeline_create(engine->standard_shader.shader);
    engine->standard_shader.u_mvp = uniform_create("u_mvp", GS_GRAPHICS_UNIFORM_MAT4, GS_GRAPHICS_SHADER_STAGE_VERTEX);
    engine->standard_shader.u_color = uniform_create("u_color", GS_GRAPHICS_UNIFORM_VEC4, GS_GRAPHICS_SHADER_STAGE_FRAGMENT);
    engine->standard_shader.u_texture = uniform_create("u_texture", GS_GRAPHICS_UNIFORM_SAMPLER2D, GS_GRAPHICS_SHADER_STAGE_FRAGMENT);

    if (!gs_asset_font_load_from_file("assets/font.otf", &engine->standard_font, 80)) {
        gs_println("WARNING: failed to load assets/font.otf (24pt)");
    }

    gs_gui_style_element_t font_style[] = {{ .type = GS_GUI_STYLE_FONT, .font = &engine->standard_font}};
    gs_gui_set_element_style(&engine->gui, GS_GUI_ELEMENT_TEXT, GS_GUI_ELEMENT_STATE_DEFAULT, font_style, sizeof(font_style));
    gs_gui_set_element_style(&engine->gui, GS_GUI_ELEMENT_TEXT, GS_GUI_ELEMENT_STATE_HOVER, font_style, sizeof(font_style));
    gs_gui_set_element_style(&engine->gui, GS_GUI_ELEMENT_TEXT, GS_GUI_ELEMENT_STATE_FOCUS, font_style, sizeof(font_style));
    gs_gui_set_element_style(&engine->gui, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_DEFAULT, font_style, sizeof(font_style));
    gs_gui_set_element_style(&engine->gui, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_HOVER, font_style, sizeof(font_style));
    gs_gui_set_element_style(&engine->gui, GS_GUI_ELEMENT_BUTTON, GS_GUI_ELEMENT_STATE_FOCUS, font_style, sizeof(font_style));
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

gs_vec2 world_to_screen(gs_camera_t *camera, gs_vec3 world_pos) {
    uint32_t fbw, fbh;
    gs_platform_framebuffer_size(gs_platform_main_window(), &fbw, &fbh);
    gs_mat4 view_proj = gs_camera_get_view_projection(camera, (int32_t)fbw, (int32_t)fbh);

    float x = world_pos.x * view_proj.m[0][0] + world_pos.y * view_proj.m[1][0] + world_pos.z * view_proj.m[2][0] + view_proj.m[3][0];
    float y = world_pos.x * view_proj.m[0][1] + world_pos.y * view_proj.m[1][1] + world_pos.z * view_proj.m[2][1] + view_proj.m[3][1];
    float z = world_pos.x * view_proj.m[0][2] + world_pos.y * view_proj.m[1][2] + world_pos.z * view_proj.m[2][2] + view_proj.m[3][2];
    float w = world_pos.x * view_proj.m[0][3] + world_pos.y * view_proj.m[1][3] + world_pos.z * view_proj.m[2][3] + view_proj.m[3][3];

    // behind the camera if w is less than or equal to 0
    if (w <= 0.0f) {
        return gs_v2(10000, 10000);
    }

    // calculate normalized device coordinates
    float ndc_x = x / w;
    float ndc_y = y / w;

    // calculate screen coords
    gs_vec2 screen;
    screen.x = ((ndc_x + 1.0f) * 0.5f) * fbw;
    screen.y = ((1.0f - ndc_y) * 0.5f) * fbh; // Inverted Y for standard 2D screen coordinate spaces

    return screen;
}

void transform_in_front_of_camera(gs_camera_t *camera, entity_t *entity, gs_vec3 pos, gs_quat rotation, float transition_speed) {
    gs_vqs transform = gs_vqs_default();
    gs_vec3 forward = gs_mat4_mul_vec3(gs_quat_to_mat4(camera->transform.rotation), gs_v3(0.f, 0.f, -1.f));
    gs_vec3 right = gs_mat4_mul_vec3(gs_quat_to_mat4(camera->transform.rotation),gs_v3(1.f, 0.f, 0.f));
    gs_vec3 up = gs_mat4_mul_vec3(gs_quat_to_mat4(camera->transform.rotation), gs_v3(0.f, 1.f, 0.f));

    transform.position = gs_vec3_add(camera->transform.position, gs_vec3_scale(forward, pos.z));
    transform.position = gs_vec3_add(transform.position, gs_vec3_scale(right, pos.x));
    transform.position = gs_vec3_add(transform.position, gs_vec3_scale(up, pos.y));

    transform.rotation = gs_quat_mul(camera->transform.rotation, gs_quat_angle_axis(gs_deg2rad(90.f), gs_v3(1.f, 0.f, 0.f)));
    transform.rotation = gs_quat_mul(transform.rotation, rotation);

    transform.scale = entity->transform.scale;

    lerp_entity_init(entity, transform, transition_speed);
}

void lerp_transform_smooth(gs_vqs *current, gs_vqs *prev, gs_vqs *next, float lerp) {
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

void lerp_transform_linear(gs_vqs *current, gs_vqs *prev, gs_vqs *next, float lerp) {
    current->position.x = gs_interp_linear(prev->position.x, next->position.x, lerp);
    current->position.y = gs_interp_linear(prev->position.y, next->position.y, lerp);
    current->position.z = gs_interp_linear(prev->position.z, next->position.z, lerp);

    current->scale.x = gs_interp_linear(prev->scale.x, next->scale.x, lerp);
    current->scale.y = gs_interp_linear(prev->scale.y, next->scale.y, lerp);
    current->scale.z = gs_interp_linear(prev->scale.z, next->scale.z, lerp);

    current->rotation.x = gs_interp_linear(prev->rotation.x, next->rotation.x, lerp);
    current->rotation.y = gs_interp_linear(prev->rotation.y, next->rotation.y, lerp);
    current->rotation.z = gs_interp_linear(prev->rotation.z, next->rotation.z, lerp);
    current->rotation.w = gs_interp_linear(prev->rotation.w, next->rotation.w, lerp);
}

void lerp_entity_init(entity_t *entity, gs_vqs target, float duration) {
    entity->prev = entity->transform;
    entity->next = target;
    entity->lerp_duration = duration;
    entity->lerp = 0.0f;
}

void lerp_entity_step(entity_t *entity, float dt) {
    if (entity->lerp_duration == 0) {
        entity->lerp = 1;
    } else {
        entity->lerp += dt / entity->lerp_duration;
        if (entity->lerp >= 1) entity->lerp = 1;
    }

    lerp_transform_smooth(&entity->transform, &entity->prev, &entity->next, entity->lerp);
}

void lerp_entities_step(gs_dyn_array(entity_t) entities, float dt) {
    for (int i = 0; i < gs_dyn_array_size(entities); i++) {
        lerp_entity_step(&entities[i], dt);
    }
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
