#pragma once

#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include "base.hpp"

struct Texture {
    using Id = usize;

    SDL_GPUTexture *texture;
    int width = 0;
    int height = 0;
};

struct Vertex2D {
    glm::vec2 position;
};

struct Vertex3D {
    Vector3 position;
    Color color;
};

struct Instance {
    enum Flags : u32 {
        USE_TEXTURE = 1u << 0,
    };

    glm::vec2 position{};
    glm::vec2 size{};
    Rectangle uv{};
    Color color = WHITE;
    u32 flags = 0;
};

struct Batch {
    u32 end;
    Texture::Id texture_id;
    Matrix4 model_view;
};

enum class Projection {
    perspective,
    orthographic,
};

struct Renderer {
    static constexpr usize MAX_TEXTURES = 128;
    static constexpr usize MAX_INSTANCES = 2048;

    SDL_Window *window;
    SDL_GPUDevice *device;

    ImGuiIO *imgui_io;

    SDL_GPUGraphicsPipeline *pipeline2d;
    SDL_GPUGraphicsPipeline *pipeline3d;
    SDL_GPUGraphicsPipeline *pipeline3d_line;
    SDL_GPUTexture *depth_texture = nullptr;
    SDL_GPUSampler *sampler;
    SDL_GPUBuffer *vertex2d_buffer;
    SDL_GPUBuffer *vertex3d_buffer;
    SDL_GPUBuffer *instance_buffer;
    SDL_GPUBuffer *index_buffer;
    SDL_GPUBuffer *vertex3d_line_buffer;
    FixedArray<Instance, MAX_INSTANCES> instances;
    IndexedArray<Texture, MAX_TEXTURES> textures;

    FixedArray<Batch, MAX_INSTANCES> batches;

    Matrix4 base_model_view;
    Matrix4 projection = Matrix4::identity();

    static constexpr usize MAX_VERTICES_3D = 2 << 10;
    FixedArray<Vertex3D, MAX_VERTICES_3D> vertices3d;
    static constexpr usize MAX_VERTICES_3D_LINE = 2;
    FixedArray<Vertex3D, MAX_VERTICES_3D_LINE> vertices3d_line;

    Vector3 camera_position = {0, 0, 0};
    Matrix4 view;
    f32 camera_pitch = 0;
    f32 camera_yaw = 0;
    f32 camera_roll = 0;

    glm::vec2 screen;
    f32 aspect_ratio;

    Projection projection_kind = Projection::perspective;

    Renderer(Allocator &gpa, SDL_Window *window, glm::vec2 screen, f32 scale_factor);

    ~Renderer() {
        SDL_WaitForGPUIdle(device);

        for (auto texture : textures) {
            SDL_ReleaseGPUTexture(device, texture.texture);
        }
        SDL_ReleaseGPUTexture(device, depth_texture);

        SDL_ReleaseGPUBuffer(device, instance_buffer);
        SDL_ReleaseGPUBuffer(device, vertex2d_buffer);
        SDL_ReleaseGPUBuffer(device, vertex3d_buffer);
        SDL_ReleaseGPUBuffer(device, index_buffer);
        SDL_ReleaseGPUBuffer(device, vertex3d_line_buffer);

        SDL_ReleaseGPUSampler(device, sampler);

        SDL_ReleaseGPUGraphicsPipeline(device, pipeline2d);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline3d);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline3d_line);

        ImGui_ImplSDL3_Shutdown();
        ImGui_ImplSDLGPU3_Shutdown();
        ImGui::DestroyContext();

        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
    }

    void frameBegin() {
        instances.clear();
        batches.clear();

        vertices3d.clear();
        vertices3d_line.clear();

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void frameEnd();

    Texture::Id loadTexture(const String &filename) {
        Texture texture{};
        auto command_buffer = SDL_AcquireGPUCommandBuffer(device);
        {
            auto copy_pass = SDL_BeginGPUCopyPass(command_buffer);

            texture.texture = IMG_LoadGPUTexture(device, copy_pass, filename.c_str.rawptr,
                                                 &texture.width, &texture.height);
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

    void drawTexture(Texture::Id texture_id, Rectangle source, Rectangle dest, Matrix4 model_view,
                     Color color = WHITE) {
        Rectangle uv = source;
        uv.x /= (f32)textures[texture_id].width;
        uv.y /= (f32)textures[texture_id].height;
        uv.w /= (f32)textures[texture_id].width;
        uv.h /= (f32)textures[texture_id].height;
        draw({.position = dest.position(),
              .size = dest.size(),
              .uv = uv,
              .color = color,
              .flags = Instance::USE_TEXTURE},
             texture_id, model_view);
    }

    void drawTriangle3D(Vector3 v1, Vector3 v2, Vector3 v3, Color color) {
        vertices3d.add({v1, color});
        vertices3d.add({v2, color});
        vertices3d.add({v3, color});
    }

    void drawPlane(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color color) {
        drawTriangle3D(v1, v2, v3, color);
        drawTriangle3D(v1, v3, v4, color);
    }

    void drawCube(Vector3 position) {
        drawPlane({-0.5f + position.x, -0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, -0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, 0.5f + position.y, -0.5f + position.z},
                  {-0.5f + position.x, 0.5f + position.y, -0.5f + position.z}, RED); // front
        drawPlane({-0.5f + position.x, -0.5f + position.y, 0.5f + position.z},
                  {0.5f + position.x, -0.5f + position.y, 0.5f + position.z},
                  {0.5f + position.x, 0.5f + position.y, 0.5f + position.z},
                  {-0.5f + position.x, 0.5f + position.y, 0.5f + position.z}, GREEN); // back
        drawPlane({-0.5f + position.x, -0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, -0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, -0.5f + position.y, 0.5f + position.z},
                  {-0.5f + position.x, -0.5f + position.y, 0.5f + position.z}, BLUE); // bottom
        drawPlane({-0.5f + position.x, 0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, 0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, 0.5f + position.y, 0.5f + position.z},
                  {-0.5f + position.x, 0.5f + position.y, 0.5f + position.z}, CYAN); // top
        drawPlane({-0.5f + position.x, 0.5f + position.y, -0.5f + position.z},
                  {-0.5f + position.x, 0.5f + position.y, 0.5f + position.z},
                  {-0.5f + position.x, -0.5f + position.y, 0.5f + position.z},
                  {-0.5f + position.x, -0.5f + position.y, -0.5f + position.z}, MAGENTA); // left
        drawPlane({0.5f + position.x, 0.5f + position.y, -0.5f + position.z},
                  {0.5f + position.x, 0.5f + position.y, 0.5f + position.z},
                  {0.5f + position.x, -0.5f + position.y, 0.5f + position.z},
                  {0.5f + position.x, -0.5f + position.y, -0.5f + position.z}, YELLOW); // right
    }

    void drawLine(Vector3 start, Vector3 end) {
        vertices3d_line.add({start, RED});
        vertices3d_line.add({end, BLUE});
    }

    void setProjection(Projection kind);
    void updateProjection();

    void resize(glm::vec2 screen_size);
};
