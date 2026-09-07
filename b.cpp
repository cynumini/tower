#include "sakana/build.cpp"

i32 main(i32 argc, const char *argv[]) {
    rebuildAndRestartOnChanges(argc, argv);

    glslc("src/shader.vert", "build/shader.vert.spv");
    glslc("src/shader.frag", "build/shader.frag.spv");

    const char *output = "./build/tower";
    const char *const input[] = {
        "src/main.cpp",
        "src/math.cpp",
        glslcHpp("src/shader.frag", "build/shader.frag.hpp", "shader_frag_code"),
        glslcHpp("src/shader.vert", "build/shader.vert.hpp", "shader_vert_code"),
        "sakana/sakana.cpp",
        "sakana/unagi.cpp",
        0,
    };
    if (needsUpdate(output, input)) {
        Args args = {};
        addArg(&args, CXX);
        addArg(&args, input[0]);

        auto compile_flags = loadFile("compile_flags.txt");
        defer(free(compile_flags.ptr));
        addArgsFromCompileFlags(&args, compile_flags);

        addArg(&args, "-o");
        addArg(&args, output);

        addArg(&args, "-lSDL3");
        addArg(&args, "-lSDL3_image");

        addArg(&args, "-g");

        run(args);
    }

    Args tidy_args = {};
    addArg(&tidy_args, "clang-tidy");
    addArg(&tidy_args, input[0]);
    run(tidy_args);

    return 0;
}
