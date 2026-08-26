#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

typedef SDL_FPoint vec2;
typedef SDL_FRect Rect;

static vec2 rectTopLeft(Rect rect) { return {rect.x, rect.y}; }
static vec2 rectTopRight(Rect rect) { return {rect.x + rect.w, rect.y}; }
static vec2 rectBotRight(Rect rect) { return {rect.x + rect.w, rect.y + rect.h}; }
static vec2 rectBotLeft(Rect rect) { return {rect.x, rect.y + rect.h}; }

static SDL_GPUShader *createGPUShader(SDL_GPUDevice *device, const char *file,
                                      SDL_GPUShaderStage stage, Uint32 num_samplers,
                                      Uint32 num_uniform_buffers) {
    size_t code_size = 0;
    auto *code = (Uint8 *)SDL_LoadFile(file, &code_size);
    SDL_assert(code);
    SDL_GPUShaderCreateInfo create_info = {};
    create_info.code_size = code_size;
    create_info.code = code;
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = stage;
    create_info.num_samplers = num_samplers;
    create_info.num_uniform_buffers = num_uniform_buffers;
    auto *shader = SDL_CreateGPUShader(device, &create_info);
    SDL_assert(shader);
    SDL_free(code);
    return shader;
}

static SDL_GPUBuffer *createGPUBuffer(SDL_GPUDevice *device, SDL_GPUBufferUsageFlags usage,
                                      Uint32 size) {
    const SDL_GPUBufferCreateInfo buffer_create_info = {usage, size, 0};
    auto *buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    SDL_assert(buffer);
    return buffer;
}

int main() {
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_assert(SDL_SetAppMetadata("tower", "0.3.0", "cynumini.tower"));
    SDL_assert(SDL_Init(SDL_INIT_VIDEO));

    const vec2 screen = {640, 360};
    auto *window = SDL_CreateWindow("tower", int(screen.x), int(screen.y), 0);
    SDL_assert(window);
    auto *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, 0);
    SDL_assert(device);
    SDL_assert(SDL_ClaimWindowForGPUDevice(device, window));

    auto *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_assert(command_buffer);
    auto *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    int width = 0;
    int height = 0;
    auto *texture = IMG_LoadGPUTexture(device, copy_pass, "resources/world.png", &width, &height);
    SDL_assert(texture);

    Rect rect0 = {0.0F, 0.0F, 16.0F, 32.0F};
    rect0.x -= (rect0.w / 2.0F);
    rect0.y -= (rect0.h / 2.0F);
    Rect rect1 = {0.0F, 32.0F, 16.0F, 32.0F};
    rect1.x /= float(width);
    rect1.w /= float(width);
    rect1.y /= float(height);
    rect1.h /= float(height);

    vec2 vertices[8] = {
        rectTopLeft(rect0),  rectTopLeft(rect1),  rectTopRight(rect0), rectTopRight(rect1),
        rectBotRight(rect0), rectBotRight(rect1), rectBotLeft(rect0),  rectBotLeft(rect1),
    };

    const Uint16 indices[6]{0, 1, 2, 0, 2, 3};

    const SDL_GPUSamplerCreateInfo sampler_create_info = {};
    auto *sampler = SDL_CreateGPUSampler(device, &sampler_create_info);

    auto *vertex_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertices));
    auto *index_buffer = createGPUBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX, sizeof(indices));

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {};
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);
    auto *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    SDL_assert(transfer_buffer);
    auto *transfer_buffer_data =
        (Uint8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    SDL_assert(transfer_buffer_data);
    SDL_memcpy(transfer_buffer_data, (Uint8 *)vertices, sizeof(vertices));
    SDL_memcpy(transfer_buffer_data + sizeof(vertices), (Uint8 *)indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUTransferBufferLocation source{transfer_buffer, 0};
    SDL_GPUBufferRegion destination = {vertex_buffer, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    source.offset = sizeof(vertices);
    destination.buffer = index_buffer;
    destination.size = sizeof(indices);
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    auto *vertex_shader =
        createGPUShader(device, "shader.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    auto *fragment_shader =
        createGPUShader(device, "shader.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.vertex_shader = vertex_shader;
    pipeline_create_info.fragment_shader = fragment_shader;
    const SDL_GPUVertexBufferDescription vertex_buffer_description = {
        0, sizeof(vec2) * 2, SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
    pipeline_create_info.vertex_input_state.vertex_buffer_descriptions =
        &vertex_buffer_description;
    pipeline_create_info.vertex_input_state.num_vertex_buffers = 1;
    const SDL_GPUVertexAttribute vertex_attribute[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 0},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, sizeof(vec2)}};
    pipeline_create_info.vertex_input_state.vertex_attributes =
        (SDL_GPUVertexAttribute *)vertex_attribute;
    pipeline_create_info.vertex_input_state.num_vertex_attributes =
        SDL_arraysize(vertex_attribute);
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(device, window);
    color_target_description.blend_state.enable_blend = true;

    color_target_description.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target_description.blend_state.dst_color_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_description.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    color_target_description.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target_description.blend_state.dst_alpha_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_description.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    pipeline_create_info.target_info.color_target_descriptions = &color_target_description;
    pipeline_create_info.target_info.num_color_targets = 1;
    auto *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    const auto *keyboard_state = SDL_GetKeyboardState(0);

    auto previous = SDL_GetTicks();
    vec2 position = {};

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

        auto current = SDL_GetTicks();
        const float dt = float(current - previous) / 1000.F;
        previous = current;

        vec2 velocity{
            float(keyboard_state[SDL_SCANCODE_D]) - float(keyboard_state[SDL_SCANCODE_A]),
            float(keyboard_state[SDL_SCANCODE_S]) - float(keyboard_state[SDL_SCANCODE_W])};

        const auto length = SDL_sqrtf((velocity.x * velocity.x) + (velocity.y * velocity.y));
        if (length > 0.0F) {
            velocity.x /= length;
            velocity.y /= length;
        }

        const auto player_speed = 100.0F;
        position.x += velocity.x * dt * player_speed;
        position.y += velocity.y * dt * player_speed;

        command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_assert(command_buffer);

        SDL_GPUTexture *swapchain_texture = 0;

        SDL_assert(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window,
                                                         &swapchain_texture, 0, 0));

        if (swapchain_texture != 0) {
            SDL_GPUColorTargetInfo color_target_info = {};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {0.5, 0.5, 0.5, 1};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
            auto *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, 0);
            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
            SDL_GPUBufferBinding buffer_binding = {vertex_buffer, 0};
            SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
            buffer_binding.buffer = index_buffer;
            SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_assert(texture);
            SDL_assert(sampler);
            const SDL_GPUTextureSamplerBinding texture_sampler_binding = {texture, sampler};
            SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
            vec2 ubo[2] = {screen, position};
            SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(ubo));
            SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);

            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    SDL_ReleaseGPUTexture(device, texture);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUSampler(device, sampler);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
