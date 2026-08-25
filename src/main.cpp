#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_GPUShader *createGPUShader(SDL_GPUDevice *device, const char *file) {
    size_t code_size = 0;
    auto *code = (Uint8 *)SDL_LoadFile(file, &code_size);
    SDL_assert(code);
    SDL_GPUShaderCreateInfo create_info = {};
    create_info.code_size = code_size;
    create_info.code = code;
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    auto *shader = SDL_CreateGPUShader(device, &create_info);
    SDL_assert(shader);
    SDL_free(code);
    return shader;
}

int main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_assert(SDL_SetAppMetadata("tower", "0.3.0", "cynumini.tower"));
    SDL_assert(SDL_Init(SDL_INIT_VIDEO));
    auto *window = SDL_CreateWindow("tower", 640, 360, 0);
    SDL_assert(window);
    auto *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
    SDL_assert(device);
    SDL_assert(SDL_ClaimWindowForGPUDevice(device, window));

    const float vertices[6] = {
        0.0F,
        1.0F, // top
        -1.0F,
        -1.0F, // left
        1.0F,
        -1.F, // right
    };
    SDL_GPUBufferCreateInfo buffer_create_info = {};
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_create_info.size = sizeof(vertices);
    auto *buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    SDL_assert(buffer);

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {};
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_create_info.size = sizeof(vertices);
    auto *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    SDL_assert(transfer_buffer);
    auto *transfer_buffer_data = (float *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    SDL_assert(transfer_buffer_data);
    SDL_memcpy(transfer_buffer_data, (float *)vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_assert(command_buffer);
    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    const SDL_GPUTransferBufferLocation source{transfer_buffer, 0};
    const SDL_GPUBufferRegion destination = {buffer, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    auto *vertex_shader = createGPUShader(device, "shader.vert.spv");
    auto *fragment_shader = createGPUShader(device, "shader.frag.spv");

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.vertex_shader = vertex_shader;
    pipeline_create_info.fragment_shader = fragment_shader;
    const SDL_GPUVertexBufferDescription vertex_buffer_description = {
        0, sizeof(float) * 2, SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
    pipeline_create_info.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_description;
    pipeline_create_info.vertex_input_state.num_vertex_buffers = 1;
    const SDL_GPUVertexAttribute vertex_attribute = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0};
    pipeline_create_info.vertex_input_state.vertex_attributes = &vertex_attribute;
    pipeline_create_info.vertex_input_state.num_vertex_attributes = 1;
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    pipeline_create_info.target_info.color_target_descriptions = &color_target_description;
    pipeline_create_info.target_info.num_color_targets = 1;
    auto *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT: {
                running = false;
            } break;
            case SDL_EVENT_KEY_DOWN: {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    running = false;
                }
            } break;
            default: {
            } break;
            }
        }

        command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_assert(command_buffer);

        SDL_GPUTexture *swapchain_texture = nullptr;

        SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture,
                                                         nullptr, nullptr));

        if (swapchain_texture != nullptr) {
            SDL_GPUColorTargetInfo color_target_info = {};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {0.5, 0.5, 0.5, 1};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            auto *render_pass =
                SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            const SDL_GPUBufferBinding buffer_binding = {buffer, 0};
            SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
            SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUBuffer(device, buffer);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
