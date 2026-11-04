#include "world_pipeline.hpp"

#include "engine.hpp"
#include "sdl.hpp"

#include "../build/shaders.hpp"

void worldResize() {
    auto &screen = engine.screen;
    engine.world.ubo.projection =
        glm::perspective(glm::radians(45.0F), float(screen.x) / float(screen.y), 0.1F, 1000.F);
}

void worldSetup() {
    auto vertex = Shader(world_vert, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    auto fragment = Shader(world_frag, SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);
    auto builder = createPipelineBuilder(vertex, fragment);
    auto vertex_slot =
        addVertexBufferDescription<WorldVertex>(builder, SDL_GPU_VERTEXINPUTRATE_VERTEX);
    ADD_VERTEX_ATTRIBUTE_SM(builder, vertex_slot, WorldVertex, pos);
    ADD_VERTEX_ATTRIBUTE_SM(builder, vertex_slot, WorldVertex, color);
    engine.world.pipeline = createPipeline(builder);
    auto size = WORLD_VERTEX_SIZE * sizeof(WorldVertex);
    engine.world.buffer = createGPUBuffer(SDL_GPU_BUFFERUSAGE_VERTEX, size);
    engine.world.transfer_buffer = createGPUTransferBuffer(size);
    engine.world.vertices.capacity = WORLD_VERTEX_SIZE;
}

void worldRelease() {
    SDL_ReleaseGPUGraphicsPipeline(engine.device, engine.world.pipeline);
    SDL_ReleaseGPUTransferBuffer(engine.device, engine.world.transfer_buffer);
    SDL_ReleaseGPUBuffer(engine.device, engine.world.buffer);
}

void worldBeginUpload(SDL_GPUCopyPass *copy_pass) {
    engine.world.vertices.ptr = (WorldVertex *)SDL_MapGPUTransferBuffer(
        engine.device, engine.world.transfer_buffer, true);

    SDL_assert(engine.world.vertices.ptr);
    engine.world.vertices.size = 0;
}

void worldEndUpload(SDL_GPUCopyPass *copy_pass) {
    SDL_UnmapGPUTransferBuffer(engine.device, engine.world.transfer_buffer);
    if (engine.world.vertices.size)
        uploadToGPUBuffer(copy_pass, engine.world.transfer_buffer, 0, 0, engine.world.buffer,
                          engine.world.vertices.size * sizeof(WorldVertex));
}

void worldDraw(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass) {
    SDL_BindGPUGraphicsPipeline(render_pass, engine.world.pipeline);

    SDL_GPUBufferBinding buffer_binding = {.buffer = engine.world.buffer, .offset = 0};
    SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);

    auto &ubo = engine.world.ubo;
    auto &camera = engine.camera;

    ubo.view = glm::rotate(glm::mat4(1.F), glm::radians(-camera.pitch), glm::vec3(1, 0, 0));
    ubo.view = glm::rotate(ubo.view, glm::radians(-camera.roll), glm::vec3(0, 1, 0));
    ubo.view = glm::rotate(ubo.view, glm::radians(-camera.yaw), glm::vec3(0, 0, 1));
    ubo.view = glm::translate(ubo.view, -camera.pos);

    SDL_PushGPUVertexUniformData(command_buffer, 0, &ubo, sizeof(WorldUBO));
    SDL_DrawGPUPrimitives(render_pass, engine.world.vertices.size, 1, 0, 0);
}

void worldColor(Color color) { engine.world.color = color; }

void worldVertex(glm::vec3 pos) {
    auto &v = engine.world.vertices;
    append(v, pos, engine.world.color);
}
