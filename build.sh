#!/usr/bin/sh

set -e
mkdir -p build
start=$EPOCHREALTIME
glslc src/shader.vert -o shader.vert.spv
glslc src/shader.frag -o shader.frag.spv
echo "glslc: took $(echo "$EPOCHREALTIME - $start" | bc) seconds"
start=$EPOCHREALTIME
flags=$(cat compile_flags.txt | tr '\n' ' ')
clang++ src/main.cpp -o build/tower $flags -g -lSDL3 -lSDL3_image
echo "clangd++: took $(echo "$EPOCHREALTIME - $start" | bc) seconds"
start=$EPOCHREALTIME
clang-tidy src/main.cpp
echo "clang-type: took $(echo "$EPOCHREALTIME - $start" | bc) seconds"
