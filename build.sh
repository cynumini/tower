#!/usr/bin/sh

set -e
mkdir -p build
clang++ src/main.cpp -o build/tower -Wall -Wpedantic -Werror
./build/tower
