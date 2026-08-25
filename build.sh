#!/usr/bin/sh

set -e
mkdir -p build
glslc src/shader.vert -o shader.vert.spv
glslc src/shader.frag -o shader.frag.spv
clang-tidy src/main.cpp
flags=$(cat compile_flags.txt | tr '\n' ' ')
clang++ src/main.cpp -o build/tower $flags -g -lSDL3
