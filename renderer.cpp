#include "renderer.hpp"

static SDL_GPUShader *loadShader(Allocator &gpa, SDL_GPUDevice *device, const String &filename, SDL_GPUShaderStage stage, u32 num_uniform_buffers = 0,
                                 u32 num_samplers = 0) {
    auto data = OS::readEntireFile(gpa, filename);
    SDL_GPUShaderCreateInfo create_info{};
    create_info.code_size = data.len;
    create_info.code = data.rawptr;
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = stage;
    create_info.num_samplers = num_samplers;
    create_info.num_uniform_buffers = num_uniform_buffers;
    auto shader = SDL_CreateGPUShader(device, &create_info);
    mem::free(gpa, data);
    return shader;
};

static SDL_GPUGraphicsPipeline *createPipeline2D(Allocator &gpa, SDL_GPUDevice *device, SDL_GPUTextureFormat texture_format) {
    SDL_GPUGraphicsPipelineCreateInfo createinfo{};

    createinfo.vertex_shader = loadShader(gpa, device, "./build/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 1);
    createinfo.fragment_shader = loadShader(gpa, device, "./build/shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);

    SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
        {.slot = 0, .pitch = sizeof(Vertex2D), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0},
        {.slot = 1, .pitch = sizeof(Instance), .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE, .instance_step_rate = 0},
    };

    SDL_GPUVertexAttribute vertex_attributes[] = {
        {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Vertex2D, position)},
        {.location = 1, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, position)},
        {.location = 2, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, size)},
        {.location = 3, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, uv)},
        {.location = 4, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Instance, uv) + sizeof(Vector2)},
        {.location = 5, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(Instance, color)},
        {.location = 6, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT, .offset = offsetof(Instance, flags)},
    };

    createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
    createinfo.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertex_buffer_descriptions);
    createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
    createinfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);

    SDL_GPUColorTargetDescription color_target_descriptions{};
    color_target_descriptions.format = texture_format;
    color_target_descriptions.blend_state.enable_blend = true;

    color_target_descriptions.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target_descriptions.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_descriptions.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    color_target_descriptions.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target_descriptions.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_descriptions.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    createinfo.target_info.color_target_descriptions = &color_target_descriptions;
    createinfo.target_info.num_color_targets = 1;

    auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
    SDL_ENSURE(pipeline, "create gpu graphics pipeline");
    SDL_ReleaseGPUShader(device, createinfo.vertex_shader);
    SDL_ReleaseGPUShader(device, createinfo.fragment_shader);
    return pipeline;
}

static SDL_GPUGraphicsPipeline *createPipeline3D(Allocator &gpa, SDL_GPUDevice *device, SDL_GPUTextureFormat texture_format) {
    SDL_GPUGraphicsPipelineCreateInfo createinfo{};

    createinfo.vertex_shader = loadShader(gpa, device, "./build/shader3d.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0);
    createinfo.fragment_shader = loadShader(gpa, device, "./build/shader3d.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
        {.slot = 0, .pitch = sizeof(Vertex3D), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0},
    };

    SDL_GPUVertexAttribute vertex_attributes[] = {
        {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = offsetof(Vertex3D, position)},
        {.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(Vertex3D, color)},
    };

    createinfo.vertex_input_state.vertex_buffer_descriptions = vertex_buffer_descriptions;
    createinfo.vertex_input_state.num_vertex_buffers = SDL_arraysize(vertex_buffer_descriptions);
    createinfo.vertex_input_state.vertex_attributes = vertex_attributes;
    createinfo.vertex_input_state.num_vertex_attributes = SDL_arraysize(vertex_attributes);

    SDL_GPUColorTargetDescription color_target_descriptions{};
    color_target_descriptions.format = texture_format;
    color_target_descriptions.blend_state.enable_blend = true;

    color_target_descriptions.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    color_target_descriptions.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_descriptions.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;

    color_target_descriptions.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    color_target_descriptions.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    color_target_descriptions.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    createinfo.depth_stencil_state.enable_depth_test = true;
    createinfo.depth_stencil_state.enable_depth_write = true;
    createinfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

    createinfo.target_info.color_target_descriptions = &color_target_descriptions;
    createinfo.target_info.num_color_targets = 1;

    createinfo.target_info.has_depth_stencil_target = true;
    createinfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

    auto pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
    SDL_ENSURE(pipeline, "create gpu graphics pipeline");
    SDL_ReleaseGPUShader(device, createinfo.vertex_shader);
    SDL_ReleaseGPUShader(device, createinfo.fragment_shader);
    return pipeline;
}

Renderer::Renderer(Allocator &gpa, SDL_Window *window, Vector2 screen, f32 scale_factor) : window(window) {
    base_model_view = Matrix4::orthographic(0, screen.x, screen.y, 0, 0, 1);

    device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, nullptr);
    SDL_ENSURE(device, "create gpu device");
    SDL_ENSURE(SDL_ClaimWindowForGPUDevice(device, window), "claim window for gpu device");
    SDL_ENSURE(SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC), "set gpu swapchain parameters");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imgui_io = &ImGui::GetIO();
    imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    imgui_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(scale_factor);
    style.FontScaleDpi = scale_factor;

    auto swapchain_texture_format = SDL_GetGPUSwapchainTextureFormat(device, window);

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info{};
    init_info.Device = device;
    init_info.ColorTargetFormat = swapchain_texture_format;
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    resize(screen);

    pipeline2d = createPipeline2D(gpa, device, swapchain_texture_format);
    pipeline3d = createPipeline3D(gpa, device, swapchain_texture_format);

    {
        SDL_GPUSamplerCreateInfo createinfo{};
        sampler = SDL_CreateGPUSampler(device, &createinfo);
        SDL_ENSURE(sampler, "create gpu sampler");
    }

    u16 indices[] = {
        0, 1, 2, 2, 1, 3,
    };

    Vertex2D vertices[] = {
        {{0.f, 0.f}},
        {{1.f, 0.f}},
        {{0.f, 1.f}},
        {{1.f, 1.f}},
    };

    {
        SDL_GPUBufferCreateInfo createinfo{};
        createinfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        createinfo.size = sizeof(vertices);
        vertex2d_buffer = SDL_CreateGPUBuffer(device, &createinfo);
        SDL_ENSURE(vertex2d_buffer, "create vertex 2d buffer");

        createinfo.size = sizeof(Vertex3D) * MAX_VERTICES_3D;
        vertex3d_buffer = SDL_CreateGPUBuffer(device, &createinfo);
        SDL_ENSURE(vertex3d_buffer, "create vertex 3d buffer");

        createinfo.size = sizeof(Instance) * MAX_INSTANCES;
        instance_buffer = SDL_CreateGPUBuffer(device, &createinfo);
        SDL_ENSURE(instance_buffer, "create instance buffer");

        createinfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        createinfo.size = sizeof(indices);
        index_buffer = SDL_CreateGPUBuffer(device, &createinfo);
        SDL_ENSURE(index_buffer, "create index buffer");
    }

    {
        SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{};
        transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transfer_buffer_create_info.size = sizeof(vertices) + sizeof(indices);

        auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
        {
            auto transfer_memory = (u8 *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
            memcpy(transfer_memory, vertices, sizeof(vertices));
            memcpy(transfer_memory + sizeof(vertices), indices, sizeof(indices));
            SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
        }
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        {
            auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);

            SDL_GPUTransferBufferLocation transfer_buffer_location{};
            transfer_buffer_location.transfer_buffer = transfer_buffer;

            SDL_GPUBufferRegion buffer_region{};

            buffer_region.buffer = vertex2d_buffer;
            buffer_region.size = sizeof(vertices);
            transfer_buffer_location.offset = 0;
            SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

            buffer_region.buffer = index_buffer;
            buffer_region.size = sizeof(indices);
            transfer_buffer_location.offset = sizeof(vertices);
            SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

            SDL_EndGPUCopyPass(copy_pass);
        }
        SDL_SubmitGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    }
}

static void upload3D(Renderer &self, SDL_GPUCommandBuffer *command_buffer) {
    if (self.vertices3d.len == 0) {
        return;
    }

    auto n = self.vertices3d.len * sizeof(Vertex3D);

    SDL_GPUTransferBufferCreateInfo createinfo{};
    createinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    createinfo.size = (u32)n;

    auto transfer_buffer = SDL_CreateGPUTransferBuffer(self.device, &createinfo);
    {
        auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(self.device, transfer_buffer, true);
        memcpy(transfer_memory, self.vertices3d.data, n);
        SDL_UnmapGPUTransferBuffer(self.device, transfer_buffer);
    }

    auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    {
        SDL_GPUTransferBufferLocation transfer_buffer_location{};
        transfer_buffer_location.transfer_buffer = transfer_buffer;

        SDL_GPUBufferRegion buffer_region{};
        buffer_region.buffer = self.vertex3d_buffer;
        buffer_region.size = (u32)n;

        SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, true);
    }
    SDL_EndGPUCopyPass(copy_pass);

    SDL_ReleaseGPUTransferBuffer(self.device, transfer_buffer);
}

static void upload2D(Renderer &self, SDL_GPUCommandBuffer *command_buffer) {
    if (self.instances.len > 0) {
        auto n = self.instances.len * sizeof(Instance);

        SDL_GPUTransferBufferCreateInfo createinfo{};
        createinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        createinfo.size = (u32)n;

        auto transfer_buffer = SDL_CreateGPUTransferBuffer(self.device, &createinfo);
        {
            auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(self.device, transfer_buffer, true);
            memcpy(transfer_memory, self.instances.data, n);
            SDL_UnmapGPUTransferBuffer(self.device, transfer_buffer);
        }

        auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
        {
            SDL_GPUTransferBufferLocation transfer_buffer_location{};
            transfer_buffer_location.transfer_buffer = transfer_buffer;

            SDL_GPUBufferRegion buffer_region{};
            buffer_region.buffer = self.instance_buffer;
            buffer_region.size = (u32)n;

            SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, true);
        }
        SDL_EndGPUCopyPass(copy_pass);

        SDL_ReleaseGPUTransferBuffer(self.device, transfer_buffer);
    }
}

static void render3D(Renderer &self, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass) {
    SDL_BindGPUGraphicsPipeline(render_pass, self.pipeline3d);

    SDL_GPUBufferBinding binding{};
    binding.buffer = self.vertex3d_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);

    auto final = self.base_model_view3d  * Matrix4::translation(-self.camera_position.x, -self.camera_position.y, -self.camera_position.z) * Matrix4::rotate({1, 0, 0}, -self.pitch) * Matrix4::rotate({0, 1, 0}, -self.yaw) * Matrix4::rotate({0, 0, 1}, -self.roll);

    SDL_PushGPUVertexUniformData(command_buffer, 0, &final, sizeof(Matrix4));

    // SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
    // texture_sampler_bindings.sampler = self.sampler;
    // SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);

    SDL_DrawGPUPrimitives(render_pass, (u32)self.vertices3d.len, 1, 0, 0);
}

static void render2D(Renderer &self, SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass) {
    SDL_BindGPUGraphicsPipeline(render_pass, self.pipeline2d);

    SDL_GPUBufferBinding binding{};
    binding.buffer = self.vertex2d_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);

    binding.buffer = self.instance_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 1, &binding, 1);

    binding.buffer = self.index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    u32 start = 0;
    for (auto batch : self.batches) {
        SDL_PushGPUVertexUniformData(command_buffer, 0, &batch.model_view, sizeof(Matrix4));
        SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
        if (batch.texture_id > 0) {
            texture_sampler_bindings.texture = self.textures[batch.texture_id].texture;
            texture_sampler_bindings.sampler = self.sampler;
            SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
        }
        u32 num_instances = batch.end - start;
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, num_instances, 0, 0, start);
        start = batch.end;
    }
}

void Renderer::frameEnd() {
    ImGui::Render();

    ImDrawData *draw_data = ImGui::GetDrawData();

    auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_ENSURE(command_buffer, "acquire gpu command buffer");

    SDL_GPUTexture *swapchain_texture;
    SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr), "wait and acquire gpu swapchain texture");

    if (swapchain_texture) {
        upload2D(*this, command_buffer);
        upload3D(*this, command_buffer);
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        SDL_GPUColorTargetInfo color_target_info{};
        color_target_info.texture = swapchain_texture;
        color_target_info.clear_color = {0.5, 0.5, 0.5, 1};
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {};
        depth_stencil_target_info.texture = depth_texture;
        depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.clear_depth = 1;

        auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);

        render3D(*this, command_buffer, render_pass);

        render2D(*this, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);

        {
            color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
            render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);

            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }
    }
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

static SDL_GPUTexture *recreateDepthTexture(SDL_GPUDevice *device, SDL_GPUTexture *texture, Vector2 screen) {
    if (texture != nullptr) {
        SDL_ReleaseGPUTexture(device, texture);
    }
    SDL_GPUTextureCreateInfo createinfo = {};
    createinfo.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    createinfo.width = (u32)screen.x;
    createinfo.height = (u32)screen.y;
    createinfo.layer_count_or_depth = 1;
    createinfo.num_levels = 1;

    printf("recreate texture %f %f\n", screen.x, screen.y);

    return SDL_CreateGPUTexture(device, &createinfo);
}

void Renderer::resize(Vector2 screen_size) {
    screen = screen_size;
    depth_texture = recreateDepthTexture(device, depth_texture, screen_size);
    aspect_ratio = screen_size.x / screen_size.y;
    base_model_view3d = Matrix4::orthographic(-10 * aspect_ratio, 10 * aspect_ratio, -10, 10, -100, 100);
}

void Renderer::setCameraForward(f32 pitch, f32 yaw) {
    
}
