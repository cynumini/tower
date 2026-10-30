#include "ui_pipeline.hpp"

#include "../build/shaders.hpp"



void UI::resize(glm::vec2 size) {
    logical.x = 640, logical.y = 360;
    if (size.x / size.y < 16.0F / 9.0F) {
        logical.y = size.y * logical.x / size.x;
    } else {
        logical.x = size.x * logical.y / size.y;
    }
    ubo.projection = glm::ortho(0.F, logical.x, logical.y, 0.F, 0.F, 1.F);
}

void UI::setup(SDL_Window *window, SDL_GPUDevice *device, SDL_GPUShaderFormat shader_format) {
    SDL_GPUGraphicsPipelineCreateInfo createinfo = {};
    createinfo.vertex_shader = createGPUShader(device, ShaderCode(ui_vert_spv, ui_vert_dxil),
                                               SDL_GPU_SHADERSTAGE_VERTEX, shader_format, 0, 1);
    SDL_assert(createinfo.vertex_shader);

    createinfo.fragment_shader =
        createGPUShader(device, ShaderCode(ui_frag_spv, ui_frag_dxil),
                        SDL_GPU_SHADERSTAGE_FRAGMENT, shader_format, 1, 0);
    SDL_assert(createinfo.fragment_shader);

    const SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
        {0, sizeof(glm::vec2), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0},
        {1, sizeof(Instance), SDL_GPU_VERTEXINPUTRATE_INSTANCE, 0}};
    createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
    createinfo.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertex_buffer_descriptions);

    SDL_GPUVertexAttribute vertex_attributes[] = {
        // vertex
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
        // instance
        {1, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, position)},
        {2, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Instance, size)},
        {3, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(Instance, uv)},
        {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Instance, color)}};
    createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
    createinfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);

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
    // defer
    SDL_ReleaseGPUShader(device, createinfo.fragment_shader);
    SDL_ReleaseGPUShader(device, createinfo.vertex_shader);
}

void UI::uploadStaticBuffers(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass) {
    glm::vec2 vertices[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    int16_t indices[6]{0, 1, 2, 0, 2, 3};

    vertex_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices));
    SDL_assert(vertex_buffer);

    index_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));
    SDL_assert(index_buffer);

    buffer =
        createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(Instance) * MAX_INSTANCES);
    SDL_assert(buffer);

    auto *transfer_buffer = createGPUTransferBuffer(device, sizeof(vertices) + sizeof(indices));

    SDL_assert(transfer_buffer);

    {
        uint8_t *memory = (uint8_t *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        SDL_assert(memory);
        SDL_memcpy(memory, vertices, sizeof(vertices));
        SDL_memcpy(memory + sizeof(vertices), indices, sizeof(indices));
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    }
    uploadToGPUBuffer(copy_pass, transfer_buffer, 0, 0, vertex_buffer, sizeof(vertices));
    uploadToGPUBuffer(copy_pass, transfer_buffer, sizeof(vertices), 0, index_buffer,
                      sizeof(indices));
    // defer
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
}

void UI::release(SDL_GPUDevice *device) {
    SDL_ReleaseGPUBuffer(device, buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
}

UI::Upload UI::beginUpload(SDL_GPUDevice *device, SDL_GPUTransferBuffer *transfer_buffer) {
    Instance *instances_raw = (Instance *)SDL_MapGPUTransferBuffer(device, transfer_buffer, true);
    SDL_assert(instances_raw);

    return {{instances_raw, MAX_INSTANCES, 0}, logical.x, logical.y};
}

void UI::endUpload(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
                   SDL_GPUTransferBuffer *transfer_buffer, size_t instances_len) {
    this->instances_len = instances_len;
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    if (this->instances_len) {
        uploadToGPUBuffer(copy_pass, transfer_buffer, 0, 0, buffer,
                          sizeof(Instance) * instances_len);
    }
}

void UI::draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, Texture atlas,
              SDL_GPUSampler *sampler) {
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

    SDL_GPUBufferBinding buffer_bindings[2] = {{vertex_buffer, 0}, {buffer, 0}};
    SDL_BindGPUVertexBuffers(render_pass, 0, buffer_bindings, 2);

    const SDL_GPUBufferBinding buffer_binding = {index_buffer, 0};
    SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    const SDL_GPUTextureSamplerBinding texture_sampler_binding = {.texture = atlas.ptr,
                                                                  .sampler = sampler};
    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);

    ubo.atlas_size = {float(atlas.w), float(atlas.h)};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(UBO));
    SDL_DrawGPUIndexedPrimitives(render_pass, 6, instances_len, 0, 0, 0);

    prev_instances_len = instances_len;
}
