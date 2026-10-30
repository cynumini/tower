#include "world_pipeline.hpp"

#include "../build/shaders.hpp"

void World::resize(FVector2 size) {
    constexpr float blocks = 15;
    logical.y = blocks;
    logical.x = logical.y * 16.0F / 9.0F;
    if (size.x / size.y < 16.0F / 9.0F) {
        logical.y = size.y * logical.x / size.x;
    } else {
        logical.x = size.x * logical.y / size.y;
    }
    ubo.projection = glm::perspective(glm::radians(60.0F), size.x / size.y, 0.1F, 1000.F);
}

void World::setup(SDL_Window *window, SDL_GPUDevice *device, SDL_GPUShaderFormat shader_format) {
    SDL_GPUGraphicsPipelineCreateInfo createinfo = {};
    createinfo.vertex_shader =
        createGPUShader(device, ShaderCode{world_vert_spv, world_vert_dxil},
                        SDL_GPU_SHADERSTAGE_VERTEX, shader_format, 0, 1);
    SDL_assert(createinfo.vertex_shader);

    createinfo.fragment_shader =
        createGPUShader(device, ShaderCode{world_frag_spv, world_frag_dxil},
                        SDL_GPU_SHADERSTAGE_FRAGMENT, shader_format, 1, 0);
    SDL_assert(createinfo.fragment_shader);

    const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
        {0, sizeof(FVector2), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
        {1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
    createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
    createinfo.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertex_buffer_descriptions);

    createinfo.rasterizer_state.enable_depth_clip = true;

    SDL_GPUVertexAttribute vertex_attributes[] = {
        // vertex
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
        // instance
        {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Instance, position)},
        {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, size)},
        {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(Instance, uv)},
        {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Instance, color)},
        {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, offsetof(Instance, face)},
        {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(Instance, rotation)}};
    createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
    createinfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);

    createinfo.depth_stencil_state = {
        .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL, // TODO: why not just less?
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    const SDL_GPUColorTargetDescription color_target_description = {
        .format = SDL_GetGPUSwapchainTextureFormat(device, window),
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
            .enable_blend = true,

        }};
    createinfo.target_info.color_target_descriptions = &color_target_description;
    createinfo.target_info.num_color_targets = 1;

    createinfo.target_info.has_depth_stencil_target = true;
    createinfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
    SDL_assert(pipeline);
    // defer
    SDL_ReleaseGPUShader(device, createinfo.fragment_shader);
    SDL_ReleaseGPUShader(device, createinfo.vertex_shader);
}

void World::uploadStaticBuffers(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass) {
    FVector2 vertices_squre[4] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}};
    FVector2 vertices_triangle[3] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}};
    int16_t indices[6]{0, 1, 2, 0, 2, 3};

    vertex_buffer_squre =
        createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices_squre));
    SDL_assert(vertex_buffer_squre);

    vertex_buffer_triangle =
        createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices_triangle));
    SDL_assert(vertex_buffer_triangle);

    index_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));
    SDL_assert(index_buffer);

    buffer =
        createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * MAX_INSTANCES);
    SDL_assert(buffer);

    auto *transfer_buffer = createGPUTransferBuffer(
        device, sizeof(vertices_squre) + sizeof(vertices_triangle) + sizeof(indices));
    SDL_assert(transfer_buffer);

    {
        uint8_t *memory = (uint8_t *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        SDL_assert(memory);

        SDL_memcpy(memory, vertices_squre, sizeof(vertices_squre));
        SDL_memcpy(memory + sizeof(vertices_squre), indices, sizeof(indices));
        SDL_memcpy(memory + sizeof(vertices_squre) + sizeof(vertices_triangle), indices,
                   sizeof(indices));
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    }
    uploadToGPUBuffer(copy_pass, transfer_buffer, 0, 0, vertex_buffer_squre,
                      sizeof(vertices_squre));
    uploadToGPUBuffer(copy_pass, transfer_buffer, 0, 0, vertex_buffer_triangle,
                      sizeof(vertices_triangle));
    uploadToGPUBuffer(copy_pass, transfer_buffer, sizeof(vertices_squre), 0, index_buffer,
                      sizeof(indices));
    // defer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}

void World::release(SDL_GPUDevice *device) {
    SDL_ReleaseGPUBuffer(device, buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device, vertex_buffer_squre);
    SDL_ReleaseGPUBuffer(device, vertex_buffer_triangle);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}

using Instances = WorldInstances;

Instances World::beginUpload(SDL_GPUDevice *device, SDL_GPUTransferBuffer *transfer_buffer) {
    Instance *instances_raw = (Instance *)SDL_MapGPUTransferBuffer(device, transfer_buffer, true);
    SDL_assert(instances_raw);

    constexpr auto half = MAX_INSTANCES / 2;

    return {.squares = FixedVector<Instance>{.ptr = instances_raw, .capacity = half},
            .triangles = FixedVector<Instance>{.ptr = instances_raw + half, .capacity = half}};
}

void World::endUpload(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
                      SDL_GPUTransferBuffer *transfer_buffer, Instances *instances) {
    squares_len = instances->squares.size;
    triangles_len = instances->triangles.size;
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    constexpr auto half = MAX_INSTANCES / 2;
    constexpr auto half_offset = half * sizeof(Instance);

    if (squares_len) {
        uploadToGPUBuffer(copy_pass, transfer_buffer, 0, 0, buffer,
                          sizeof(Instance) * squares_len);
    }
    if (triangles_len) {
        uploadToGPUBuffer(copy_pass, transfer_buffer, half_offset, half_offset, buffer,
                          sizeof(Instance) * triangles_len);
    }
}

void World::draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                 Texture atlas, SDL_GPUSampler *sampler, Camera *camera) {
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

    SDL_GPUBufferBinding buffer_bindings[2] = {{vertex_buffer_squre, 0}, {buffer, 0}};
    SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings, 2);

    const SDL_GPUBufferBinding buffer_binding = {index_buffer, 0};
    SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    const SDL_GPUTextureSamplerBinding texture_sampler_binding = {.texture = atlas.ptr,
                                                                  .sampler = sampler};
    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);


    ubo.view = glm::rotate(glm::mat4(1.F), glm::radians(-camera->pitch), glm::vec3(1, 0, 0));
    ubo.view = glm::rotate(ubo.view, glm::radians(-camera->roll), glm::vec3(0, 1, 0));
    ubo.view = glm::rotate(ubo.view, glm::radians(-camera->yaw), glm::vec3(0, 0, 1));
    ubo.view = glm::translate(ubo.view, {-camera->pos.x, -camera->pos.y, -camera->pos.z - 1});

    ubo.yaw = deg2rad(camera->yaw);
    ubo.atlas_size = {float(atlas.w), float(atlas.h)};

    SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
    SDL_DrawGPUIndexedPrimitives(render_pass, 6, squares_len, 0, 0, 0);

    // triangle
    buffer_bindings[0] = {vertex_buffer_triangle, 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings, 2);
    SDL_DrawGPUPrimitives(render_pass, 3, triangles_len, 0, MAX_INSTANCES / 2);

    prev_squares_len = squares_len;
    prev_triangles_len = triangles_len;
}
