#include "sakana/build.cpp"

i32 main(i32 argc, const char *argv[]) {
    sakanaBuildSubproject("sakana");
    sakanaBuild(argc, argv);

    glslc("src/shader.vert", "build/shader.vert.spv");
    glslc("src/shader.frag", "build/shader.frag.spv");

    const char *output = "./build/tower";
    const char *const src[] = {"src/main.cpp", "sakana/build/sakana.o", "src/math.cpp",
                               "src/root.hpp", 0};
    const char *sakana = "sakana/build/sakana.o";
    if (needsUpdate(output, src) or needsUpdate(output, sakana)) {
        Args args = {};
        addArg(&args, CXX);
        addArg(&args, src[0]);
        addArg(&args, sakana);

        auto compile_flags = loadFile("compile_flags.txt");
        defer(free(compile_flags.ptr));
        addArgsFromCompileFlags(&args, compile_flags);

        addArg(&args, "-g");
        addArg(&args, "-lSDL3");
        addArg(&args, "-lSDL3_image");
        addArg(&args, "-o");
        addArg(&args, output);
        run(args.data);
    }

    const char *tidy_args[] = {"clang-tidy", src[0], 0};
    run(tidy_args);
}
