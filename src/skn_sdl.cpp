#pragma once

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <span>

#define SDL_CHECK2(cond, file, line)                                                             \
    do {                                                                                         \
        if (!(cond)) {                                                                           \
            SDL_Log("%s:%d: error: %s", file, line, SDL_GetError());                             \
            return SDL_APP_FAILURE;                                                              \
        }                                                                                        \
    } while (false)

#define SDL_CHECK(cond) SDL_CHECK2(cond, __FILE__, __LINE__) // NOLINT

constexpr uint8_t MAX_TEXTURE_SAMPLERS = 16;

struct ShaderCode {
    const std::span<const uint8_t> spv;
    const std::span<const uint8_t> dxil;
};

static SDL_GPUShader *createGPUShader(SDL_GPUDevice *device, ShaderCode code,
                                      SDL_GPUShaderStage stage, SDL_GPUShaderFormat shader_format,
                                      uint num_samplers, uint num_uniform_buffers) {
    SDL_GPUShaderCreateInfo createinfo = {};
    if (shader_format == SDL_GPU_SHADERFORMAT_SPIRV) {
        createinfo.code_size = code.spv.size();
        createinfo.code = code.spv.data();
    } else if (shader_format == SDL_GPU_SHADERFORMAT_DXIL) {
        createinfo.code_size = code.dxil.size();
        createinfo.code = code.dxil.data();
    } else {
        SDL_assert(false and "SPIRV and DXIL only");
    }
    createinfo.entrypoint = "main";
    createinfo.format = shader_format;
    createinfo.stage = stage;
    createinfo.num_samplers = num_samplers;
    createinfo.num_uniform_buffers = num_uniform_buffers;
    return SDL_CreateGPUShader(device, &createinfo);
};

static SDL_GPUBuffer *createGPUBuffer(SDL_GPUDevice *device, SDL_GPUBufferUsageFlags usage,
                                      Uint32 size) {
    const SDL_GPUBufferCreateInfo createinfo = {usage, size, 0};
    return SDL_CreateGPUBuffer(device, &createinfo);
};

static SDL_GPUTransferBuffer *createGPUTransferBuffer(SDL_GPUDevice *device, uint size) {
    const SDL_GPUTransferBufferCreateInfo createinfo = {SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, size,
                                                        0};
    return SDL_CreateGPUTransferBuffer(device, &createinfo);
}

static void uploadToGPUBuffer(SDL_GPUCopyPass *copy_pass, SDL_GPUTransferBuffer *transfer_buffer,
                              uint in_offset, uint out_offset, SDL_GPUBuffer *buffer, uint size) {
    const SDL_GPUTransferBufferLocation source = {transfer_buffer, in_offset};
    const SDL_GPUBufferRegion destination = {buffer, out_offset, size};
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);
}

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
