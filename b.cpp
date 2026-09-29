#include "sakana/skn_build.cpp"

int main(int argc, const char *argv[]) {
    auto arena = Arena::init(MB(1));
    REBUILD_AND_RESTART_ON_CHANGES(&arena, argc, argv);

    const char *output = "./build/tower";
    const char *inputs[] = {
        "engine.cpp",
        // "unagi.cpp",
        // "game.cpp",
        // "object.cpp",
        // "game.hpp",
        // "unagi.hpp",
        // "sakana/skn.cpp",
        // "sakana/skn_math.cpp",
        // "sakana/skn_sdl.cpp",
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

    // Args tidy_args = {};
    // addArg(&tidy_args, "clang-tidy");
    // addArg(&tidy_args, inputs[0]); // engine
    // addArg(&tidy_args, inputs[1]); // game
    // run(tidy_args);

    return 0;
}
