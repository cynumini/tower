#pragma once

#include "color.hpp"

#include <glm/glm.hpp>

#include <SDL3_image/SDL_image.h>

// Old
#define SDL_CHECK2(cond, file, line)                                                             \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            SDL_Log("%s:%d: error: %s", file, line, SDL_GetError());                             \
            return SDL_APP_FAILURE;                                                              \
        }                                                                                        \
    } while (false)

#define SDL_CHECK(cond) SDL_CHECK2(cond, __FILE__, __LINE__) // NOLINT

constexpr uint8_t MAX_TEXTURE_SAMPLERS = 16;

static SDL_GPUShader *createGPUShader(SDL_GPUDevice *device, std::span<const uint8_t> code,
                                      SDL_GPUShaderStage stage, SDL_GPUShaderFormat shader_format,
                                      uint num_samplers, uint num_uniform_buffers) {
    SDL_GPUShaderCreateInfo createinfo{};
    createinfo.code_size = code.size();
    createinfo.code = code.data();
    createinfo.entrypoint = "main";
    createinfo.format = shader_format;
    createinfo.stage = stage;
    createinfo.num_samplers = num_samplers;
    createinfo.num_uniform_buffers = num_uniform_buffers;
    return SDL_CreateGPUShader(device, &createinfo);
};



struct Texture {
    int w, h;
    SDL_GPUTexture *ptr;

    static Texture create(SDL_GPUDevice *device, int w, int h) {
        const SDL_GPUTextureCreateInfo createinfo = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = uint(w),
            .height = uint(h),
            .layer_count_or_depth = 1,
            .num_levels = 1,
        };
        auto *ptr = SDL_CreateGPUTexture(device, &createinfo);
        SDL_assert(ptr);
        return {w, h, ptr};
    }

    static Texture load(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass, const char *file) {
        int w, h;
        auto *ptr = IMG_LoadGPUTexture(device, copy_pass, file, &w, &h);
        SDL_assert(ptr);
        return {w, h, ptr};
    }

    static Texture load(SDL_GPUDevice *device, SDL_GPUCopyPass *copy_pass,
                        std::span<uint8_t> data) {
        int w, h;
        auto *src = SDL_IOFromConstMem(data.data(), data.size());
        auto *ptr = IMG_LoadGPUTexture_IO(device, copy_pass, src, true, &w, &h);
        SDL_assert(ptr);
        return {w, h, ptr};
    }

    void uploadToGPU(SDL_GPUCopyPass *copy_pass, SDL_GPUTransferBuffer *transfer_buffer, uint x,
                     uint y, uint w, uint h) const {
        const SDL_GPUTextureTransferInfo source = {
            .transfer_buffer = transfer_buffer,
            .offset = 0,
            .pixels_per_row = 0,
            .rows_per_layer = 0,
        };
        const SDL_GPUTextureRegion destination = {
            .texture = ptr,
            .x = x,
            .y = y,
            .w = w,
            .h = h,
            .d = 1,
        };
        SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
    }
};

// Shader
struct Shader {
    SDL_GPUShader *ptr;
    Shader(std::span<const std::uint8_t> code, SDL_GPUShaderStage stage, uint32_t num_samplers,
           uint32_t num_uniform_buffers);
    ~Shader();
};

// Pipeline
struct PipelineBuilder {
    SDL_GPUShader *vertex;
    SDL_GPUShader *fragment;
    std::vector<SDL_GPUVertexBufferDescription> vertex_buffer_descriptions;
    std::vector<SDL_GPUVertexAttribute> vertex_attributes;
};

PipelineBuilder createPipelineBuilder(const Shader &vertex, const Shader &fragment);

template <typename T>
uint32_t addVertexBufferDescription(PipelineBuilder &self, SDL_GPUVertexInputRate input_rate) {
    uint32_t slot = self.vertex_buffer_descriptions.size();
    self.vertex_buffer_descriptions.emplace_back(slot, sizeof(T), input_rate, 0);
    return slot;
}

void add_vertex_attribute_format(PipelineBuilder &self, uint32_t buffer_slot,
                                 SDL_GPUVertexElementFormat format, uint32_t offset);

template <typename T>
void add_vertex_attribute(PipelineBuilder &self, uint32_t buffer_slot, uint32_t offset) {
    uint32_t location = self.vertex_attributes.size();
    SDL_GPUVertexElementFormat format = {};
    if constexpr (std::is_same_v<T, float>) {
        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    } else if constexpr (std::is_same_v<T, SDL_FRect>) {
        format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    } else if constexpr (std::is_same_v<T, Color>) {
        format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    } else {
        static_assert(false, "Type T is not implemented");
    }
    self.vertex_attributes.emplace_back(location, buffer_slot, format, offset);
}

/// type + offset
#define ADD_VERTEX_ATTRIBUTE_TO(self, buffer_slot, t, offset)                                    \
    add_vertex_attribute<t>(self, buffer_slot, offset)

/// struct + memeber
#define ADD_VERTEX_ATTRIBUTE_SM(self, buffer_slot, s, m)                                         \
    add_vertex_attribute<decltype(s::m)>(self, buffer_slot, offsetof(s, m))

/// struct + memeber + custom format
#define ADD_VERTEX_ATTRIBUTE_SMF(self, buffer_slot, format, s, m)                                \
    add_vertex_attribute_format(self, buffer_slot, format, offsetof(s, m))

SDL_GPUGraphicsPipeline *createPipeline(const PipelineBuilder &builder);

// TransferBuffer
SDL_GPUTransferBuffer *createGPUTransferBuffer(uint32_t size);

void uploadToGPUBuffer(SDL_GPUCopyPass *copy_pass, SDL_GPUTransferBuffer *transfer_buffer,
                       uint32_t in_offset, uint32_t out_offset, SDL_GPUBuffer *buffer,
                       uint32_t size);

// Buffer
SDL_GPUBuffer *createGPUBuffer(SDL_GPUBufferUsageFlags usage, Uint32 size);
