#include "voxel_pipeline.hpp"

#include "engine.hpp"
#include "sdl.hpp"

#include "../build/shaders.hpp"

void Voxel::resize(glm::vec2 size) {
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

void Voxel::uploadStaticBuffers(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass) {
    glm::vec2 vertices_squre[4] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}};
    glm::vec2 vertices_triangle[3] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}};
    int16_t indices[6]{0, 1, 2, 0, 2, 3};

    vertex_buffer_squre =
        createGPUBuffer(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices_squre));
    SDL_assert(vertex_buffer_squre);

    vertex_buffer_triangle =
        createGPUBuffer(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices_triangle));
    SDL_assert(vertex_buffer_triangle);

    index_buffer = createGPUBuffer(SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));
    SDL_assert(index_buffer);

    buffer =
        createGPUBuffer(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * MAX_INSTANCES);
    SDL_assert(buffer);

    auto *transfer_buffer = createGPUTransferBuffer(sizeof(vertices_squre) +
                                                    sizeof(vertices_triangle) + sizeof(indices));
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

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}

void Voxel::release(SDL_GPUDevice *device) {
    SDL_ReleaseGPUBuffer(device, buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device, vertex_buffer_squre);
    SDL_ReleaseGPUBuffer(device, vertex_buffer_triangle);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}

using Instances = WorldInstances;

Instances Voxel::beginUpload(SDL_GPUDevice *device, SDL_GPUTransferBuffer *transfer_buffer) {
    Instance *instances_raw = (Instance *)SDL_MapGPUTransferBuffer(device, transfer_buffer, true);
    SDL_assert(instances_raw);

    constexpr auto half = MAX_INSTANCES / 2;

    return {.squares = FixedVector<Instance>{.ptr = instances_raw, .capacity = half},
            .triangles = FixedVector<Instance>{.ptr = instances_raw + half, .capacity = half}};
}

void Voxel::endUpload(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
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

void Voxel::draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
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
    ubo.view = glm::translate(ubo.view, {-camera->pos.x, -camera->pos.y, -camera->pos.z - 0.8});

    ubo.yaw = glm::radians(camera->yaw);
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

void voxelSetup(Voxel &self, SDL_GPUShaderFormat shader_format) {
    auto vertex = Shader(voxel_vert, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    auto fragment = Shader(voxel_frag, SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);
    auto builder = createPipelineBuilder(vertex, fragment);
    auto vertex_slot =
        addVertexBufferDescription<glm::vec2>(builder, SDL_GPU_VERTEXINPUTRATE_VERTEX);
    auto instance_slot =
        addVertexBufferDescription<VoxelInstance>(builder, SDL_GPU_VERTEXINPUTRATE_INSTANCE);
    ADD_VERTEX_ATTRIBUTE_TO(builder, vertex_slot, glm::vec2, 0);
    ADD_VERTEX_ATTRIBUTE_SM(builder, instance_slot, VoxelInstance, position);
    ADD_VERTEX_ATTRIBUTE_SM(builder, instance_slot, VoxelInstance, size);
    ADD_VERTEX_ATTRIBUTE_SM(builder, instance_slot, VoxelInstance, uv);
    ADD_VERTEX_ATTRIBUTE_SM(builder, instance_slot, VoxelInstance, color);
    ADD_VERTEX_ATTRIBUTE_SMF(builder, instance_slot, SDL_GPU_VERTEXELEMENTFORMAT_UINT,
                             VoxelInstance, face);
    ADD_VERTEX_ATTRIBUTE_SM(builder, instance_slot, VoxelInstance, rotation);
    self.pipeline = createPipeline(builder);
}
