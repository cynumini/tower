#pragma once

#include "color.hpp"
#include "fixed_vector.hpp"
#include "skn_sdl.cpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

struct UIInstance {
    glm::vec2 position;
    glm::vec2 size;
    SDL_FRect uv;
    Color color;
};

struct UI {
    using Instance = UIInstance;

    static constexpr uint MAX_INSTANCES = 512;

    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    SDL_GPUBuffer *buffer;

    uint instances_len;
    uint prev_instances_len;

    glm::vec2 logical;

    struct UBO {
        glm::mat4 projection;
        glm::vec2 atlas_size;
    } ubo;

    void resize(glm::vec2 size);

    void setup(SDL_Window *window, SDL_GPUDevice *device, SDL_GPUShaderFormat shader_format);

    void uploadStaticBuffers(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass);

    void release(SDL_GPUDevice *device);

    struct Upload {
        FixedVector<Instance> instances;
        float width, height;
    };

    Upload beginUpload(SDL_GPUDevice *device, SDL_GPUTransferBuffer *transfer_buffer);

    void endUpload(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
                   SDL_GPUTransferBuffer *transfer_buffer, size_t instances_len);

    void draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, Texture atlas,
              SDL_GPUSampler *sampler);
};

extern UI ui;
