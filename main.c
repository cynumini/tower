#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include <cglm/cglm.h>

#define WHITE (SDL_Color){255, 255, 255, 255}
#define GRAY (SDL_Color){128, 128, 128, 255}
#define BLACK (SDL_Color){0, 0, 0, 255}

#define RED (SDL_Color){255, 0, 0, 255}
#define ORANGE (SDL_Color){255, 165, 0, 255}
#define YELLOW (SDL_Color){255, 255, 0, 255}
#define LIME (SDL_Color){191, 255, 0, 255}
#define GREEN (SDL_Color){0, 255, 0, 255}
#define TEAL (SDL_Color){0, 128, 128, 255}
#define CYAN (SDL_Color){0, 255, 255, 255}
#define BLUE (SDL_Color){0, 0, 255, 255}
#define NAVY (SDL_Color){0, 0, 128, 255}
#define PURPLE (SDL_Color){128, 0, 128, 255}
#define MAGENTA (SDL_Color){255, 0, 255, 255}
#define PINK (SDL_Color){255, 192, 203, 255}
#define BROWN (SDL_Color){165, 42, 42, 255}

static SDL_FColor toFColor(SDL_Color color) {
    return (SDL_FColor){(float)color.r / 255.0F, (float)color.g / 255.0F, (float)color.b / 255.0F,
                        (float)color.a / 255.0F};
}

typedef struct WorldVertex {
    vec3 pos;
    SDL_Color color;
} WorldVertex;

typedef struct WorldUBO {
    mat4 view;
    mat4 projection;
} WorldUBO;

typedef struct World {
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *buffer;
    SDL_GPUTransferBuffer *transfer_buffer;
    SDL_Color color;

    // camera
    vec3 pos;
    float pitch;
    float roll;
    float yaw;

    WorldVertex *vertices;
    uint32_t vertices_len;

    WorldUBO ubo;
} World;

#define WORLD_VERTEX_LEN 8

typedef struct Engine {
    int width;
    int height;

    SDL_Window *window;
    SDL_GPUDevice *device;
    SDL_GPUTexture *depth;

    // input
    const bool *keyboard_state;
    float mouse_sensitivity;

    World world;
} Engine;

static Engine engine; // NOLINT

static SDL_GPUShader *loadShader(const char *path, SDL_GPUShaderStage stage,
                                 uint32_t num_samplers, uint32_t num_uniform_buffers) {
    FILE *file = fopen(path, "r");
    SDL_assert(file);
    SDL_assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    SDL_assert(size != -1);
    SDL_assert(fseek(file, 0, SEEK_SET) == 0);
    uint8_t *data = (uint8_t *)malloc(size * sizeof(char));
    SDL_assert(data);
    SDL_assert(fread(data, sizeof(char), size, file) == size);
    SDL_GPUShaderCreateInfo createinfo = {.code_size = size,
                                          .code = data,
                                          .entrypoint = "main",
                                          .stage = stage,
                                          .format = SDL_GPU_SHADERFORMAT_SPIRV,
                                          .num_samplers = num_samplers,
                                          .num_uniform_buffers = num_uniform_buffers};
    SDL_GPUShader *ptr = SDL_CreateGPUShader(engine.device, &createinfo);
    SDL_assert(ptr);
    free(data);
    return ptr;
}

static void resize() {
    glm_perspective(glm_rad(45.F), (float)engine.width / (float)engine.height, 0.1F, 1000.F,
                    engine.world.ubo.projection);
    if (engine.depth) SDL_ReleaseGPUTexture(engine.device, engine.depth);
    SDL_GPUTextureCreateInfo createinfo = {
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = engine.width,
        .height = engine.height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    engine.depth = SDL_CreateGPUTexture(engine.device, &createinfo);
    SDL_assert(engine.depth);
}

static void worldColor(SDL_Color color) { engine.world.color = color; }

void worldVertex(vec3 pos) {
    SDL_assert(engine.world.vertices_len < WORLD_VERTEX_LEN);
    WorldVertex *vertex = &engine.world.vertices[engine.world.vertices_len++];
    glm_vec3_copy(pos, vertex->pos);
    vertex->color = engine.world.color;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    const char *name = "tower";

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetAppMetadata(name, "0.5.0", "cynumini.tower");

    SDL_assert(SDL_Init(SDL_INIT_VIDEO));

    engine.world.pos[2] = 0;
    engine.width = 1280;
    engine.height = 720;
    engine.mouse_sensitivity = 0.15F;
    engine.window = SDL_CreateWindow(name, engine.width, engine.height, SDL_WINDOW_RESIZABLE);
    SDL_assert(engine.window);

    engine.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_assert(engine.device);

    SDL_assert(SDL_ClaimWindowForGPUDevice(engine.device, engine.window));

    {
        SDL_GPUSamplerCreateInfo createinfo = {};
        SDL_GPUSampler *sampler = SDL_CreateGPUSampler(engine.device, &createinfo);
        SDL_assert(sampler);
    }

    {
        SDL_GPUTextureCreateInfo createinfo = {.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                               .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
                                               .width = 4096,
                                               .height = 4096,
                                               .layer_count_or_depth = 1,
                                               .num_levels = 1};
        SDL_GPUTexture *atlas = SDL_CreateGPUTexture(engine.device, &createinfo);
        SDL_assert(atlas);
    }

    {
        SDL_GPUShader *vertex_shader =
            loadShader("build/world.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
        SDL_GPUShader *fragment_shader =
            loadShader("build/world.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

        SDL_GPUVertexBufferDescription vertex_buffer_description = {
            .slot = 0,
            .pitch = sizeof(WorldVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX};

        SDL_GPUVertexAttribute vertex_attributes[] = {
            {.location = 0,
             .buffer_slot = 0,
             .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
             .offset = 0},
            {.location = 1,
             .buffer_slot = 0,
             .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
             .offset = offsetof(WorldVertex, color)}};

        const SDL_GPUColorTargetDescription color_target_description = {
            .format = SDL_GetGPUSwapchainTextureFormat(engine.device, engine.window),
            .blend_state = {
                .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
                .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .color_blend_op = SDL_GPU_BLENDOP_ADD,
                .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                .enable_blend = true,
            }};

        SDL_GPUGraphicsPipelineCreateInfo createinfo = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state = {.vertex_buffer_descriptions = &vertex_buffer_description,
                                   .num_vertex_buffers = 1,
                                   .vertex_attributes = vertex_attributes,
                                   .num_vertex_attributes = SDL_arraysize(vertex_attributes)},
            .rasterizer_state = {.enable_depth_clip = true},
            .depth_stencil_state = {.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
                                    .enable_depth_test = true,
                                    .enable_depth_write = true},
            .target_info = {.color_target_descriptions = &color_target_description,
                            .num_color_targets = 1,
                            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                            .has_depth_stencil_target = true}};

        engine.world.pipeline = SDL_CreateGPUGraphicsPipeline(engine.device, &createinfo);
        SDL_assert(engine.world.pipeline);

        SDL_ReleaseGPUShader(engine.device, fragment_shader);
        SDL_ReleaseGPUShader(engine.device, vertex_shader);
    }

    uint32_t buffer_size = WORLD_VERTEX_LEN * sizeof(WorldVertex);
    {
        SDL_GPUBufferCreateInfo createinfo = {.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                              .size = buffer_size};
        engine.world.buffer = SDL_CreateGPUBuffer(engine.device, &createinfo);
        SDL_assert(engine.world.buffer);
    }
    {
        SDL_GPUTransferBufferCreateInfo createinfo = {.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                      .size = buffer_size};
        engine.world.transfer_buffer = SDL_CreateGPUTransferBuffer(engine.device, &createinfo);
        SDL_assert(engine.world.transfer_buffer);
    }

    resize();

    engine.keyboard_state = SDL_GetKeyboardState(NULL);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_WINDOW_RESIZED: {
        engine.width = event->window.data1;
        engine.height = event->window.data2;
        resize();
    } break;
    case SDL_EVENT_QUIT: {
        return SDL_APP_SUCCESS;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        SDL_SetWindowRelativeMouseMode(engine.window,
                                       !SDL_GetWindowRelativeMouseMode(engine.window));
    } break;
    case SDL_EVENT_MOUSE_MOTION: {
        engine.world.yaw -= event->motion.xrel * engine.mouse_sensitivity;
        engine.world.pitch -= event->motion.yrel * engine.mouse_sensitivity;

    } break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(engine.device);
    SDL_assert(command_buffer);

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    engine.world.vertices = (WorldVertex *)SDL_MapGPUTransferBuffer(
        engine.device, engine.world.transfer_buffer, true);
    SDL_assert(engine.world.vertices);
    engine.world.vertices_len = 0;
    // begin draw
    worldColor(RED);
    worldVertex((vec3){-1.5F, -1.5F, -5});
    worldVertex((vec3){1.5F, -1.5F, -5});
    worldVertex((vec3){0.0F, 1.5F, -5});

    worldColor(GREEN);
    worldVertex((vec3){-1.5F, -1.5F, -4});
    worldVertex((vec3){1.5F, -1.5F, -4});
    worldVertex((vec3){0.0F, 1.5F, -4});

    // end draw
    SDL_UnmapGPUTransferBuffer(engine.device, engine.world.transfer_buffer);
    engine.world.vertices = NULL;
    if (engine.world.vertices_len) {
        SDL_GPUTransferBufferLocation source = {.transfer_buffer = engine.world.transfer_buffer,
                                                0};
        SDL_GPUBufferRegion destination = {.buffer = engine.world.buffer,
                                           .offset = 0,
                                           .size =
                                               engine.world.vertices_len * sizeof(WorldVertex)};
        SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);
    }

    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUTexture *swapchain_texture = NULL;
    uint width = 0;
    uint height = 0;
    bool result = SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, engine.window,
                                                        &swapchain_texture, &width, &height);
    if (!result) {
        SDL_Log("error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (engine.width != width || engine.height != height) {
        resize();
        engine.width = width;
        engine.height = height;
    }

    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target_info = {
            .texture = swapchain_texture,
            .clear_color = toFColor(WHITE),
        };
        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
            .texture = engine.depth,
            .clear_depth = 1.0F,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };

        SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(
            command_buffer, &color_target_info, 1, &depth_stencil_target_info);

        SDL_BindGPUGraphicsPipeline(render_pass, engine.world.pipeline);
        SDL_GPUBufferBinding buffer_binding = {.buffer = engine.world.buffer, .offset = 0};
        SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);

        World *world = &engine.world;
        WorldUBO *ubo = &world->ubo;

        glm_mat4_identity(ubo->view);
        glm_rotate(ubo->view, glm_rad(-world->pitch), (vec3){1, 0, 0});
        glm_rotate(ubo->view, glm_rad(-world->roll), (vec3){0, 1, 0});
        glm_rotate(ubo->view, glm_rad(-world->yaw), (vec3){0, 0, 1});
        vec3 neg_pos;
        glm_vec3_negate_to(world->pos, neg_pos);
        glm_translate(ubo->view, neg_pos);

        SDL_PushGPUVertexUniformData(command_buffer, 0, &engine.world.ubo, sizeof(WorldUBO));

        SDL_DrawGPUPrimitives(render_pass, engine.world.vertices_len, 1, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    SDL_DestroyGPUDevice(engine.device);
    SDL_DestroyWindow(engine.window);
    SDL_Quit();
}
