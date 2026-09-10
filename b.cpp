#include "sakana/skn_build.cpp"

int main(int argc, const char *argv[]) {
    rebuildAndRestartOnChanges(argc, argv);

    const char *output = "./build/tower";
    const char *const inputs[] = {
        "unagi.cpp",
        "game.cpp",
        "sakana/skn.cpp",
        "sakana/skn_math.cpp",
        "sakana/skn_sdl.cpp",
        glslcHpp("shader.frag", "./build/shader.frag.hpp", "shader_frag_code"),
        glslcHpp("shader.vert", "./build/shader.vert.hpp", "shader_vert_code"),
        0,
    };
    if (needsUpdate(output, inputs)) {
        Args args = {};
        addArg(&args, CXX);
        addArg(&args, inputs[0]); // engine
        addArg(&args, inputs[1]); // game

        auto compile_flags = loadFile("compile_flags.txt");

        addArgsFromCompileFlags(&args, compile_flags);

        addArg(&args, "-o");
        addArg(&args, output);

        addArg(&args, "-lSDL3");
        addArg(&args, "-lSDL3_image");

        addArg(&args, "-g");

        run(args);

        free(compile_flags.ptr);
    }

    Args tidy_args = {};
    addArg(&tidy_args, "clang-tidy");
    addArg(&tidy_args, inputs[0]); // engine
    addArg(&tidy_args, inputs[1]); // game
    run(tidy_args);

    return 0;
}
