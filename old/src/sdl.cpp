#include "sdl.hpp"

#include "engine.hpp"

Shader::Shader(std::span<const std::uint8_t> code, SDL_GPUShaderStage stage,
               uint32_t num_samplers, uint32_t num_uniform_buffers) {
    const SDL_GPUShaderCreateInfo createinfo = {.code_size = code.size(),
                                                .code = code.data(),
                                                .entrypoint = "main",
                                                .format = SHADER_FORMAT,
                                                .stage = stage,
                                                .num_samplers = num_samplers,
                                                .num_storage_textures = 0,
                                                .num_storage_buffers = 0,
                                                .num_uniform_buffers = num_uniform_buffers,
                                                .props = 0};
    ptr = SDL_CreateGPUShader(engine.device, &createinfo);
}

Shader::~Shader() { SDL_ReleaseGPUShader(engine.device, ptr); }

PipelineBuilder createPipelineBuilder(const Shader &vertex, const Shader &fragment) {
    return {.vertex = vertex.ptr,
            .fragment = fragment.ptr,
            .vertex_buffer_descriptions = {},
            .vertex_attributes = {}};
}

void add_vertex_attribute_format(PipelineBuilder &self, uint32_t buffer_slot,
                                 SDL_GPUVertexElementFormat format, uint32_t offset) {
    uint32_t location = self.vertex_attributes.size();
    self.vertex_attributes.emplace_back(location, buffer_slot, format, offset);
}

SDL_GPUGraphicsPipeline *createPipeline(const PipelineBuilder &builder) {
    const SDL_GPUColorTargetDescription color_target_description = {
        .format = SDL_GetGPUSwapchainTextureFormat(engine.device, engine.window),
        .blend_state = {
            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
            .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .color_blend_op = SDL_GPU_BLENDOP_ADD,
            .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
            .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
            .color_write_mask = 0,
            .enable_blend = true,
            .enable_color_write_mask = false,
            .padding1 = 0,
            .padding2 = 0,
        }};
    const SDL_GPUGraphicsPipelineCreateInfo createinfo = {
        .vertex_shader = builder.vertex,
        .fragment_shader = builder.fragment,
        .vertex_input_state =
            {.vertex_buffer_descriptions = builder.vertex_buffer_descriptions.data(),
             .num_vertex_buffers = uint32_t(builder.vertex_buffer_descriptions.size()),
             .vertex_attributes = builder.vertex_attributes.data(),
             .num_vertex_attributes = uint32_t(builder.vertex_attributes.size())},
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = {.fill_mode = SDL_GPU_FILLMODE_FILL,
                             .cull_mode = SDL_GPU_CULLMODE_NONE,
                             .front_face = SDL_GPU_FRONTFACE_CLOCKWISE,
                             .depth_bias_constant_factor = 0.F,
                             .depth_bias_clamp = 0.F,
                             .depth_bias_slope_factor = 0.F,
                             .enable_depth_bias = false,
                             .enable_depth_clip = true,
                             .padding1 = 0,
                             .padding2 = 0},
        .multisample_state = {.sample_count = SDL_GPU_SAMPLECOUNT_1,
                              .sample_mask = 0,
                              .enable_mask = false,
                              .enable_alpha_to_coverage = false,
                              .padding2 = 0,
                              .padding3 = 0},
        .depth_stencil_state = {.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
                                .back_stencil_state = {.fail_op = SDL_GPU_STENCILOP_INVALID,
                                                       .pass_op = SDL_GPU_STENCILOP_INVALID,
                                                       .depth_fail_op = SDL_GPU_STENCILOP_INVALID,
                                                       .compare_op = SDL_GPU_COMPAREOP_INVALID},
                                .front_stencil_state = {.fail_op = SDL_GPU_STENCILOP_INVALID,
                                                        .pass_op = SDL_GPU_STENCILOP_INVALID,
                                                        .depth_fail_op =
                                                            SDL_GPU_STENCILOP_INVALID,
                                                        .compare_op = SDL_GPU_COMPAREOP_INVALID},
                                .compare_mask = 0,
                                .write_mask = 0,
                                .enable_depth_test = true,
                                .enable_depth_write = true,
                                .enable_stencil_test = false,
                                .padding1 = 0,
                                .padding2 = 2,
                                .padding3 = 0},
        .target_info = {.color_target_descriptions = &color_target_description,
                        .num_color_targets = 1,
                        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                        .has_depth_stencil_target = true,
                        .padding1 = 0,
                        .padding2 = 0,
                        .padding3 = 0},
        .props = 0,
    };
    auto pipeline = SDL_CreateGPUGraphicsPipeline(engine.device, &createinfo);
    SDL_assert(pipeline);
    return pipeline;
}

SDL_GPUTransferBuffer *createGPUTransferBuffer(uint32_t size) {
    const SDL_GPUTransferBufferCreateInfo createinfo = {SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, size,
                                                        0};
    auto self = SDL_CreateGPUTransferBuffer(engine.device, &createinfo);
    SDL_assert(self);
    return self;

}

void uploadToGPUBuffer(SDL_GPUCopyPass *copy_pass, SDL_GPUTransferBuffer *transfer_buffer,
                       uint in_offset, uint out_offset, SDL_GPUBuffer *buffer, uint size) {
    const SDL_GPUTransferBufferLocation source = {transfer_buffer, in_offset};
    const SDL_GPUBufferRegion destination = {buffer, out_offset, size};
    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);
}

SDL_GPUBuffer *createGPUBuffer(SDL_GPUBufferUsageFlags usage, Uint32 size) {
    const SDL_GPUBufferCreateInfo createinfo = {usage, size, 0};
    return SDL_CreateGPUBuffer(engine.device, &createinfo);
};
