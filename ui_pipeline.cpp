#pragma once

#include <skn_sdl.cpp>

#include "shared.cpp"

#include "build/ui.frag.hpp"
#include "build/ui.vert.hpp"

static struct UI {
    using Instance = UIInstance;

    static constexpr uint MAX_INSTANCES = 256;

    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    SDL_GPUBuffer *buffer;

    uint instances_len;
    uint prev_instances_len;

    float logical_w, logical_h;

    struct UBO {
        Mat4 projection;
    } ubo;

    void resize(float w, float h) {
        logical_w = 640, logical_h = 360;
        if (w / h < 16.0F / 9.0F) {
            logical_h = h * logical_w / w;
        } else {
            logical_w = w * logical_h / h;
        }
        ubo.projection = Mat4::ortho(0, logical_w, logical_h, 0, 0, 1);
    }

    void setup(SDL_Window *window, SDL_GPUDevice *device, SDL_GPUShaderFormat shader_format) {
        SDL_GPUGraphicsPipelineCreateInfo createinfo = {};
        createinfo.vertex_shader =
            createGPUShader(device, {ui_vert_code_spv, ui_vert_code_dxil},
                            SDL_GPU_SHADERSTAGE_VERTEX, shader_format, 0, 1);
        defer(SDL_ReleaseGPUShader(device, createinfo.vertex_shader));
        SDL_assert(createinfo.vertex_shader);

        createinfo.fragment_shader =
            createGPUShader(device, {ui_frag_code_spv, ui_frag_code_dxil},
                            SDL_GPU_SHADERSTAGE_FRAGMENT, shader_format, 1, 0);
        defer(SDL_ReleaseGPUShader(device, createinfo.fragment_shader));
        SDL_assert(createinfo.fragment_shader);

        const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
            {0, sizeof(Vec2), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
            {1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
        createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
        createinfo.vertex_input_state.num_vertex_buffers = ARRAY_LEN(vertex_buffer_descriptions);

        SDL_GPUVertexAttribute vertex_attributes[] = {
            // vertex
            {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
            // instance
            {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, position)},
            {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, size)},
            {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(Instance, uv)},
            {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Instance, color)}};
        createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
        createinfo.vertex_input_state.num_vertex_attributes = ARRAY_LEN(vertex_attributes);

        createinfo.depth_stencil_state = {
            .enable_depth_test = false,
            .enable_depth_write = false,
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
        createinfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

        pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
        SDL_assert(pipeline);
    }

    void uploadStaticBuffers(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass) {
        Vec2 vertices[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        i16 indices[6]{0, 1, 2, 0, 2, 3};

        vertex_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices));
        SDL_assert(vertex_buffer);

        index_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));
        SDL_assert(index_buffer);

        buffer =
            createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * MAX_INSTANCES);
        SDL_assert(buffer);

        auto *transfer_buffer =
            createGPUTransferBuffer(device, sizeof(vertices) + sizeof(indices));
        defer(SDL_ReleaseGPUTransferBuffer(device, transfer_buffer));
        SDL_assert(transfer_buffer);

        {
            u8 *memory = (u8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
            defer(SDL_UnmapGPUTransferBuffer(device, transfer_buffer));
            SDL_assert(memory);

            SDL_memcpy(memory, vertices, sizeof(vertices));
            SDL_memcpy(memory + sizeof(vertices), indices, sizeof(indices));
        }
        uploadToGPUBuffer(copy_pass, transfer_buffer, 0, vertex_buffer, sizeof(vertices));
        uploadToGPUBuffer(copy_pass, transfer_buffer, sizeof(vertices), index_buffer,
                          sizeof(indices));
    }

    void release(SDL_GPUDevice *device) {
        SDL_ReleaseGPUBuffer(device, buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }

    struct Upload {
        Fixed<Instance> instances;
        float width, height;
    };

    Upload beginUpload(SDL_GPUDevice *device, SDL_GPUTransferBuffer *transfer_buffer) {
        Instance *instances_raw =
            (Instance *)SDL_MapGPUTransferBuffer(device, transfer_buffer, true);
        SDL_assert(instances_raw);

        return {Fixed<Instance>::init({MAX_INSTANCES, instances_raw}), logical_w, logical_h};
    }

    void endUpload(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
                   SDL_GPUTransferBuffer *transfer_buffer, size_t instances_len) {
        this->instances_len = instances_len;
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        if (this->instances_len) {
            uploadToGPUBuffer(copy_pass, transfer_buffer, 0, buffer,
                              sizeof(Instance) * instances_len);
        }
    }

    void draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
              SDL_GPUTexture *atlas, SDL_GPUSampler *sampler) {
        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

        SDL_GPUBufferBinding buffer_bindings[2] = {{vertex_buffer, 0}, {buffer, 0}};
        SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings, 2);

        const SDL_GPUBufferBinding buffer_binding = {index_buffer, 0};
        SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        const SDL_GPUTextureSamplerBinding texture_sampler_binding = {.texture = atlas,
                                                                      .sampler = sampler};
        SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);

        SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, instances_len, 0, 0, 0);

        prev_instances_len = instances_len;
    }
} ui;
