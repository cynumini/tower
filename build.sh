#!/usr/bin/bash

set -e

ROOT=$(pwd)
SDL_PATH="./subprojects/SDL"

if [ -d "$SDL_PATH/build" ]; then
  echo "$0: SDL is already built."
else
    cd $SDL_PATH
    cmake -S . -B build
    cmake --build build
    cd $ROOT
fi

SDL_FLAGS=("-L$SDL_PATH/build" "-lSDL3")

export LD_LIBRARY_PATH=$SDL_PATH/build:$LD_LIBRARY_PATH

mkdir -p ./build
clang++ main.cpp ${SDL_FLAGS[@]} -o ./build/tower
./build/tower
