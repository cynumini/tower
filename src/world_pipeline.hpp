#pragma once

#include "color.hpp"
#include "fixed_vector.hpp"
#include "skn_sdl.cpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

enum class Face : uint32_t { y_pos, x_pos, y_neg, x_neg, z_pos, z_neg, billboard, slope };

struct Camera {
    float pitch = 60;
    float yaw = 45;
    float roll = 0;
    glm::vec3 pos;
};

struct WorldInstance {
    glm::vec3 position;
    glm::vec2 size;
    SDL_FRect uv;
    Color color;
    Face face;
    float rotation;
};

struct WorldInstances {
    FixedVector<WorldInstance> squares;
    FixedVector<WorldInstance> triangles;
};

struct World {
    using Instance = WorldInstance;

    static constexpr uint MAX_INSTANCES = 8192;

    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *vertex_buffer_squre;
    SDL_GPUBuffer *vertex_buffer_triangle;
    SDL_GPUBuffer *index_buffer;
    SDL_GPUBuffer *buffer;

    uint squares_len;
    uint triangles_len;
    uint prev_squares_len;
    uint prev_triangles_len;

    glm::vec2 logical;

    struct UBO {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec2 atlas_size;
        float yaw;
    } ubo;

    void resize(glm::vec2 size);
    void setup(SDL_Window *window, SDL_GPUDevice *device, SDL_GPUShaderFormat shader_format);
    void uploadStaticBuffers(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass);
    void release(SDL_GPUDevice *device);
    WorldInstances beginUpload(SDL_GPUDevice *device, SDL_GPUTransferBuffer *transfer_buffer);
    void endUpload(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
                   SDL_GPUTransferBuffer *transfer_buffer, WorldInstances *instances);
    void draw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, Texture atlas,
              SDL_GPUSampler *sampler, Camera *camera);
};
extern World world;
