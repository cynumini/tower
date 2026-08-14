#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include "base.cpp"

struct Texture {
    using Id = usize;

    SDL_GPUTexture *texture;
    int width = 0;
    int height = 0;
};

struct Renderer {
    static constexpr usize MAX_TEXTURES = 128;
    static constexpr usize MAX_INSTANCES = 2048;

    struct Vertex {
        Vector2 position;
    };

    struct Instance {
        enum Flags : u32 {
            USE_TEXTURE = 1u << 0,
        };

        Vector2 position{};
        Vector2 size{};
        Rectangle uv{};
        Color color = WHITE;
        u32 flags = 0;
    };

    struct Batch {
        u32 end;
        Texture::Id texture_id;
        Matrix4 model_view;
    };

    SDL_Window *window;
    SDL_GPUDevice *device;

    ImGuiIO *imgui_io;

    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUSampler *sampler;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *instance_buffer;
    SDL_GPUBuffer *index_buffer;
    FixedArray<Instance, MAX_INSTANCES> instances;
    IndexedArray<Texture, MAX_TEXTURES> textures;

    FixedArray<Batch, MAX_INSTANCES> batches;

    Matrix4 base_model_view;

    Renderer(SDL_Window *window, Vector2 screen, f32 scale_factor) : window(window) {
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

        {
            auto load_shader = [this](const String &filename, SDL_GPUShaderStage stage, u32 num_uniform_buffers = 0, u32 num_samplers = 0) {
                auto data = OS::readEntireFile(filename);
                SDL_GPUShaderCreateInfo create_info{};
                create_info.code_size = data.len;
                create_info.code = data.rawptr;
                create_info.entrypoint = "main";
                create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
                create_info.stage = stage;
                create_info.num_samplers = num_samplers;
                create_info.num_uniform_buffers = num_uniform_buffers;
                auto shader = SDL_CreateGPUShader(device, &create_info);
                data.deinit();
                return shader;
            };
            SDL_GPUGraphicsPipelineCreateInfo createinfo{};

            createinfo.vertex_shader = load_shader("./build/shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 1);
            createinfo.fragment_shader = load_shader("./build/shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 1);

            SDL_GPUVertexBufferDescription vertex_buffer_descriptions[] = {
                {.slot = 0, .pitch = sizeof(Vertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0},
                {.slot = 1, .pitch = sizeof(Instance), .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE, .instance_step_rate = 0},
            };

            SDL_GPUVertexAttribute vertex_attributes[] = {
                {.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(Vertex, position)},
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
            color_target_descriptions.format = swapchain_texture_format;
            color_target_descriptions.blend_state.enable_blend = true;

            color_target_descriptions.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
            color_target_descriptions.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target_descriptions.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;

            color_target_descriptions.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
            color_target_descriptions.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            color_target_descriptions.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

            createinfo.target_info.color_target_descriptions = &color_target_descriptions;
            createinfo.target_info.num_color_targets = 1;

            pipeline = SDL_CreateGPUGraphicsPipeline(device, &createinfo);
            SDL_ENSURE(pipeline, "create gpu graphics pipeline");
            SDL_ReleaseGPUShader(device, createinfo.vertex_shader);
            SDL_ReleaseGPUShader(device, createinfo.fragment_shader);
        }

        {
            SDL_GPUSamplerCreateInfo createinfo{};
            sampler = SDL_CreateGPUSampler(device, &createinfo);
            SDL_ENSURE(sampler, "create gpu sampler");
        }

        u16 indices[] = {
            0, 1, 2, 2, 1, 3,
        };

        Vertex vertices[] = {
            {{0.f, 0.f}},
            {{1.f, 0.f}},
            {{0.f, 1.f}},
            {{1.f, 1.f}},
        };

        {
            SDL_GPUBufferCreateInfo createinfo{};
            createinfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            createinfo.size = sizeof(vertices);
            vertex_buffer = SDL_CreateGPUBuffer(device, &createinfo);
            SDL_ENSURE(vertex_buffer, "create vertex buffer");

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

                buffer_region.buffer = vertex_buffer;
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

    ~Renderer() {
        for (auto texture : textures) {
            SDL_ReleaseGPUTexture(device, texture.texture);
        }
        SDL_WaitForGPUIdle(device);

        SDL_ReleaseGPUBuffer(device, instance_buffer);
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);

        SDL_ReleaseGPUSampler(device, sampler);

        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);

        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();

        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }

    void frameBegin() {
        instances.clear();
        batches.clear();

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void frameEnd() {
        ImGui::Render();

        ImDrawData *draw_data = ImGui::GetDrawData();

        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        SDL_ENSURE(command_buffer, "acquire gpu command buffer");

        SDL_GPUTexture *swapchain_texture;
        SDL_ENSURE(SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr),
                   "wait and acquire gpu swapchain texture");

        if (swapchain_texture) {
            if (instances.len > 0) {
                auto n = instances.len * sizeof(Instance);

                SDL_GPUTransferBufferCreateInfo createinfo{};
                createinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
                createinfo.size = (u32)n;

                auto transfer_buffer = SDL_CreateGPUTransferBuffer(device, &createinfo);
                {
                    auto transfer_memory = (char *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
                    memcpy(transfer_memory, instances.data, n);
                    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
                }

                auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);
                {
                    SDL_GPUTransferBufferLocation transfer_buffer_location{};
                    transfer_buffer_location.transfer_buffer = transfer_buffer;

                    SDL_GPUBufferRegion buffer_region{};
                    buffer_region.buffer = instance_buffer;
                    buffer_region.size = (u32)n;

                    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, true);
                }
                SDL_EndGPUCopyPass(copy_pass);
                SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
            }

            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            SDL_GPUColorTargetInfo color_target_info{};
            color_target_info.texture = swapchain_texture;
            color_target_info.clear_color = {0.5, 0.5, 0.5, 1};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;

            auto render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);

            SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

            SDL_GPUBufferBinding binding{};
            binding.buffer = vertex_buffer;
            SDL_BindGPUVertexBuffers(render_pass, 0, &binding, 1);

            binding.buffer = instance_buffer;
            SDL_BindGPUVertexBuffers(render_pass, 1, &binding, 1);

            binding.buffer = index_buffer;
            SDL_BindGPUIndexBuffer(render_pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            u32 start = 0;
            for (auto batch : batches) {
                SDL_PushGPUVertexUniformData(command_buffer, 0, &batch.model_view, sizeof(Matrix4));
                SDL_GPUTextureSamplerBinding texture_sampler_bindings{};
                if (batch.texture_id > 0) {
                    texture_sampler_bindings.texture = textures[batch.texture_id].texture;
                    texture_sampler_bindings.sampler = sampler;
                    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_bindings, 1);
                }
                u32 num_instances = batch.end - start;
                SDL_DrawGPUIndexedPrimitives(render_pass, 6, num_instances, 0, 0, start);
                start = batch.end;
            }

            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }
        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    Texture::Id loadTexture(const String &filename) {
        Texture texture{};
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        {
            auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);

            texture.texture = IMG_LoadGPUTexture(device, copy_pass, filename.c_str, &texture.width, &texture.height);
            SDL_ENSURE(texture.texture, "load gpu texture");

            SDL_EndGPUCopyPass(copy_pass);
        }
        SDL_SubmitGPUCommandBuffer(command_buffer);

        return textures.add(texture);
    };

    void draw(const Instance &instance, Texture::Id texture_id, Matrix4 model_view) {
        instances.add(instance);
        if (batches.len == 0) {
            batches.add({1, texture_id, model_view});
            return;
        }
        auto &batch = batches.last();

        // Since instances without a texture don't care whether the
        // batch has one or not, assign the texture to a batch that
        // initially had no texture.
        if (batch.texture_id == 0) {
            batch.texture_id = texture_id;
        }

        if (batch.texture_id == texture_id and batch.model_view == model_view) {
            batch.end++;
        } else {
            batches.add({batch.end + 1, texture_id, model_view});
        }
    }

    void drawRectangle(const Rectangle &dest, const Matrix4 &model_view, const Color &color) {
        draw({.position = dest.position(), .size = dest.size(), .color = color}, 0, model_view);
    }

    void drawTexture(Texture::Id texture_id, Rectangle source, Rectangle dest, Matrix4 model_view, Color color = WHITE) {
        Rectangle uv = source;
        uv.x /= (f32)textures[texture_id].width;
        uv.y /= (f32)textures[texture_id].height;
        uv.w /= (f32)textures[texture_id].width;
        uv.h /= (f32)textures[texture_id].height;
        draw({.position = dest.position(), .size = dest.size(), .uv = uv, .color = color, .flags = Instance::USE_TEXTURE}, texture_id, model_view);
    }
};
