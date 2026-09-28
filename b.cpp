#include "sakana/skn_build.cpp"

int main(int argc, const char *argv[]) {
    auto ctx = Context::init();
    REBUILD_AND_RESTART_ON_CHANGES(ctx, argc, argv);

    const char *output = "./build/tower";
    const char *inputs[] = {
        "unagi.cpp",
        "game.cpp",
        "object.cpp",
        "game.hpp",
        "unagi.hpp",
        "sakana/skn.cpp",
        "sakana/skn_math.cpp",
        "sakana/skn_sdl.cpp",
        glslcHpp(ctx, "shader.frag", "./build/shader.frag.hpp", "shader_frag_code"),
        glslcHpp(ctx, "shader.vert", "./build/shader.vert.hpp", "shader_vert_code"),
    };
    if (needsUpdate(output, inputs)) {
        auto args = Dynamic<const char *>::init(&ctx.arena, 32);

        args.append(&ctx.arena, CXX);
        args.append(&ctx.arena, inputs[0]); // engine
        args.append(&ctx.arena, inputs[1]); // game

        addArgsFromCompileFlags(ctx, &args);

        args.append(&ctx.arena, "-o");
        args.append(&ctx.arena, output);

        args.append(&ctx.arena, "-lSDL3");
        args.append(&ctx.arena, "-lSDL3_image");

        args.append(&ctx.arena, "-g");

        run(ctx, args);
    }

    // Args tidy_args = {};
    // addArg(&tidy_args, "clang-tidy");
    // addArg(&tidy_args, inputs[0]); // engine
    // addArg(&tidy_args, inputs[1]); // game
    // run(tidy_args);

    return 0;
}
