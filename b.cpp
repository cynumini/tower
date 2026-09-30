#include "sakana/skn_build.cpp"

int main(int argc, const char *argv[]) {
    auto arena = Arena::init(MB(1));
    REBUILD_AND_RESTART_ON_CHANGES(&arena, argc, argv);
    const char *output = "./build/tower";
    const char *inputs[] = {
        "engine.cpp",
        "game.cpp",
        "tower.hpp",
        "ui_pipeline.cpp",
        shadercrossHpp(&arena, "shader.frag.hlsl", "./build/shader.frag.hpp", "shader_frag_code"),
        shadercrossHpp(&arena, "shader.vert.hlsl", "./build/shader.vert.hpp", "shader_vert_code"),
    };
    if (needsUpdate(output, inputs)) {
        auto args = Dynamic<const char *>::init(&arena, 32);

        args.append(&arena, CXX);
        args.append(&arena, inputs[0]); // engine

        addArgsFromCompileFlags(&arena, &args);

        args.append(&arena, "-o");
        args.append(&arena, output);

        args.append(&arena, "-lSDL3");
        args.append(&arena, "-lSDL3_image");

        args.append(&arena, "-g");

        run(&arena, args);
    }
    return 0;
}
