#include "sakana/skn_build.cpp"

int main(int argc, const char *argv[]) {
    auto arena = Arena::init(MB(1));
    REBUILD_AND_RESTART_ON_CHANGES(&arena, argc, argv);
    const char *output = "./build/tower";
    const char *inputs[] = {
        "main.cpp",
        "game.cpp",
        "map.cpp",
        "shared.cpp",
        "ui_pipeline.cpp",
        "world_pipeline.cpp",
        shadercrossHpp(&arena, "ui.frag.hlsl", "./build/ui.frag.hpp", "ui_frag_code"),
        shadercrossHpp(&arena, "ui.vert.hlsl", "./build/ui.vert.hpp", "ui_vert_code"),
        shadercrossHpp(&arena, "world.frag.hlsl", "./build/world.frag.hpp", "world_frag_code"),
        shadercrossHpp(&arena, "world.vert.hlsl", "./build/world.vert.hpp", "world_vert_code"),
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
        // args.append(&arena, "-O3");

        run(&arena, args);
    }
    return 0;
}
