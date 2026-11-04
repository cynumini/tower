#pragma once

#include <glm/glm.hpp>

#include "color.hpp"
#include "fixed_vector.hpp"

constexpr size_t WORLD_VERTEX_SIZE = 512;

struct WorldVertex {
    glm::vec3 pos;
    Color color;
};

struct WorldUBO {
    glm::mat4 view;
    glm::mat4 projection;
};

struct World {
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUBuffer *buffer;
    WorldUBO ubo;
    FixedVector<WorldVertex> vertices;
    SDL_GPUTransferBuffer *transfer_buffer;
    Color color = WHITE;
};

void worldResize();
void worldSetup();
void worldRelease();
void worldBeginUpload(SDL_GPUCopyPass *copy_pass);
void worldEndUpload(SDL_GPUCopyPass *copy_pass);
void worldDraw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass);
void worldColor(Color color);
void worldVertex(glm::vec3 pos);
