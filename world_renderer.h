#ifndef WORLD_RENDERER_H
#define WORLD_RENDERER_H

#include "gs.h"

typedef struct mesh_t {
    gs_handle(gs_graphics_vertex_buffer_t) vbo;
    gs_handle(gs_graphics_index_buffer_t) ibo;
    uint32_t index_count;
} mesh_t;


typedef struct {
    gs_command_buffer_t cb;
    gs_camera_t camera;

    gs_handle(gs_graphics_shader_t) shader;
    gs_handle(gs_graphics_pipeline_t) pipeline;
    gs_handle(gs_graphics_uniform_t) u_mvp;
    gs_handle(gs_graphics_uniform_t) u_color;

    mesh_t ground;
    mesh_t sphere;

    gs_vec3 player_pos;
    float player_radius;
    float player_speed;

    gs_vec3 camera_offset;
    float ground_half_size;
} world_render_data_t;

world_render_data_t world_renderer = {0};

static mesh_t upload_mesh(gs_vec3* verts, uint32_t vert_count, uint16_t* indices, uint32_t index_count) {
    mesh_t m = gs_default_val();

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

static mesh_t build_ground_mesh(float half_size) {
    gs_vec3 verts[4] = {
        gs_v3(-half_size, 0.f, -half_size),
        gs_v3( half_size, 0.f, -half_size),
        gs_v3( half_size, 0.f,  half_size),
        gs_v3(-half_size, 0.f,  half_size),
    };

    uint16_t indices[6] = {
        0, 2, 1,
        0, 3, 2
    };

    return upload_mesh(verts, 4, indices, 6);
}

static mesh_t build_sphere_mesh(float radius, int32_t stacks, int32_t slices) {
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

    mesh_t m = upload_mesh(verts, gs_dyn_array_size(verts), indices, gs_dyn_array_size(indices));

    gs_dyn_array_free(verts);
    gs_dyn_array_free(indices);
    return m;
}

static gs_handle(gs_graphics_shader_t) build_shader(void) {
    const char* vsrc =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;\n"
    "uniform mat4 u_mvp;\n"
    "void main() {\n"
    "    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
    "}\n";

    const char* fsrc =
    "#version 330 core\n"
    "uniform vec4 u_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = u_color;\n"
    "}\n";

    gs_graphics_shader_source_desc_t sources[2] = gs_default_val();
    sources[0].type = GS_GRAPHICS_SHADER_STAGE_VERTEX;
    sources[0].source = vsrc;
    sources[1].type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT;
    sources[1].source = fsrc;

    gs_graphics_shader_desc_t sdesc = gs_default_val();
    sdesc.sources = sources;
    sdesc.size = sizeof(sources);
    memcpy(sdesc.name, "basic_mvp_color_shader", sizeof("basic_mvp_color_shader"));

    return gs_graphics_shader_create(&sdesc);
}

static gs_handle(gs_graphics_uniform_t)
build_uniform(const char* name,
              gs_graphics_uniform_type type,
              gs_graphics_shader_stage_type stage)
{
    gs_graphics_uniform_layout_desc_t layout = gs_default_val();
    layout.type = type;

    gs_graphics_uniform_desc_t desc = gs_default_val();
    desc.stage = stage;
    desc.layout = &layout;
    desc.layout_size = sizeof(layout);

    memcpy(desc.name, name, strlen(name) + 1);

    return gs_graphics_uniform_create(&desc);
}


static gs_handle(gs_graphics_pipeline_t) build_pipeline(gs_handle(gs_graphics_shader_t) shader) {
    gs_graphics_vertex_attribute_desc_t attrs[1] = gs_default_val();
    attrs[0].format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3;
    memcpy(attrs[0].name, "a_position", sizeof("a_position"));

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

static void draw_mesh(mesh_t* mesh, gs_mat4 model, gs_mat4 view_projection, gs_vec4 color)
{
    gs_mat4 mvp = gs_mat4_mul(view_projection, model);

    gs_graphics_bind_vertex_buffer_desc_t vb = gs_default_val();
    vb.buffer = mesh->vbo;

    gs_graphics_bind_index_buffer_desc_t ib = gs_default_val();
    ib.buffer = mesh->ibo;

    gs_graphics_bind_uniform_desc_t uniforms[2] = gs_default_val();
    uniforms[0].uniform = world_renderer.u_mvp;
    uniforms[0].data = &mvp;
    uniforms[1].uniform = world_renderer.u_color;
    uniforms[1].data = &color;

    gs_graphics_bind_desc_t binds = gs_default_val();
    binds.vertex_buffers.desc = &vb;
    binds.vertex_buffers.size = sizeof(vb);
    binds.index_buffers.desc = &ib;
    binds.index_buffers.size = sizeof(ib);
    binds.uniforms.desc = uniforms;
    binds.uniforms.size = sizeof(uniforms);

    gs_graphics_apply_bindings(&world_renderer.cb, &binds);

    gs_graphics_draw_desc_t draw = gs_default_val();
    draw.start = 0;
    draw.count = mesh->index_count;
    gs_graphics_draw(&world_renderer.cb, &draw);
}

void world_init(void)
{
    world_renderer.cb = gs_command_buffer_new();

    world_renderer.shader = build_shader();
    world_renderer.pipeline = build_pipeline(world_renderer.shader);

    world_renderer.u_mvp =
    build_uniform("u_mvp",
                  GS_GRAPHICS_UNIFORM_MAT4,
                  GS_GRAPHICS_SHADER_STAGE_VERTEX);

    world_renderer.u_color =
    build_uniform("u_color",
                  GS_GRAPHICS_UNIFORM_VEC4,
                  GS_GRAPHICS_SHADER_STAGE_FRAGMENT);

    world_renderer.ground_half_size = 15.f;
    world_renderer.ground = build_ground_mesh(world_renderer.ground_half_size);

    world_renderer.player_radius = 0.5f;
    world_renderer.sphere =
    build_sphere_mesh(world_renderer.player_radius, 16, 24);

    world_renderer.player_pos =
    gs_v3(0.f, world_renderer.player_radius, 0.f);

    world_renderer.player_speed = 6.f;
    world_renderer.camera_offset = gs_v3(0.f, 7.f, 6.f);

    // IMPORTANT: create a perspective camera.
    // world_renderer.camera = gs_camera_perspective();
    world_renderer.camera = gs_camera_default();
    world_renderer.camera.proj_type = GS_PROJECTION_TYPE_ORTHOGRAPHIC;
    world_renderer.camera.ortho_scale = 10.f;

    world_renderer.camera.transform.position =
    gs_vec3_add(world_renderer.player_pos,
                world_renderer.camera_offset);

    world_renderer.camera.transform.rotation =
    gs_quat_angle_axis(
        gs_deg2rad(-30.f),
                       gs_v3(1.f, 0.f, 0.f)
    );

    world_renderer.camera.transform.scale = gs_v3s(1.f);

    world_renderer.camera.fov = 60.f;
    world_renderer.camera.near_plane = 0.1f;
    world_renderer.camera.far_plane = 1000.f;
}


void world_update(void) {
    float dt = gs_platform_delta_time();

    // ---- WASD movement ----
    gs_vec3 move = gs_v3(0.f, 0.f, 0.f);
    if (gs_platform_key_down(GS_KEYCODE_W)) move.z -= 1.f;
    if (gs_platform_key_down(GS_KEYCODE_S)) move.z += 1.f;
    if (gs_platform_key_down(GS_KEYCODE_A)) move.x -= 1.f;
    if (gs_platform_key_down(GS_KEYCODE_D)) move.x += 1.f;

    if (gs_vec3_len(move) > 0.f)
    {
        move = gs_vec3_norm(move);
        move = gs_vec3_scale(move, world_renderer.player_speed * dt);
        world_renderer.player_pos = gs_vec3_add(world_renderer.player_pos, move);
    }

    float half = world_renderer.ground_half_size - world_renderer.player_radius;
    world_renderer.player_pos.x = gs_clamp(world_renderer.player_pos.x, -half, half);
    world_renderer.player_pos.z = gs_clamp(world_renderer.player_pos.z, -half, half);

    // Camera only ever translates - rotation was set once in app_init.
    world_renderer.camera.transform.position = gs_vec3_add(world_renderer.player_pos, world_renderer.camera_offset);


    gs_vec2 fbs = gs_platform_framebuffer_sizev(gs_platform_main_window());
    gs_mat4 vp = gs_camera_get_view_projection(&world_renderer.camera, (uint32_t)fbs.x, (uint32_t)fbs.y);

    gs_graphics_clear_action_t clear_action = gs_default_val();
    clear_action.flag = GS_GRAPHICS_CLEAR_COLOR | GS_GRAPHICS_CLEAR_DEPTH;
    clear_action.color[0] = 0.08f;
    clear_action.color[1] = 0.08f;
    clear_action.color[2] = 0.10f;
    clear_action.color[3] = 1.f;

    gs_graphics_clear_desc_t clear = gs_default_val();
    clear.actions = &clear_action;
    clear.size = sizeof(clear_action);

    gs_graphics_renderpass_begin(&world_renderer.cb, (gs_handle(gs_graphics_renderpass_t)){0});
    gs_graphics_clear(&world_renderer.cb, &clear);
    gs_graphics_set_viewport(&world_renderer.cb, 0, 0, (uint32_t)fbs.x, (uint32_t)fbs.y);
    gs_graphics_pipeline_bind(&world_renderer.cb, world_renderer.pipeline);

    gs_vqs ground_xform = gs_default_val();
    ground_xform.position = gs_v3(0.f, 0.f, 0.f);
    ground_xform.rotation = gs_quat_default();
    ground_xform.scale = gs_v3s(1.f);
    draw_mesh(&world_renderer.ground, gs_vqs_to_mat4(&ground_xform), vp, gs_v4(0.31f, 0.55f, 0.31f, 1.f));

    gs_vqs sphere_xform = gs_default_val();
    sphere_xform.position = world_renderer.player_pos;
    sphere_xform.rotation = gs_quat_default();
    sphere_xform.scale = gs_v3s(1.f);
    draw_mesh(&world_renderer.sphere, gs_vqs_to_mat4(&sphere_xform), vp, gs_v4(0.86f, 0.24f, 0.24f, 1.f));
    gs_graphics_renderpass_end(&world_renderer.cb);

    gs_graphics_command_buffer_submit(&world_renderer.cb);
}

#endif
