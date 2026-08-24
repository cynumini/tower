cmake_minimum_required(VERSION 3.23)
project(sakana)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED)

add_library(core src/builtin.cpp)
add_library(unagi src/unagi.cpp)

target_include_directories(core PUBLIC include)
target_include_directories(unagi PUBLIC include)

target_link_libraries(unagi PRIVATE core SDL3::SDL3)

add_library(sakana::unagi ALIAS unagi)
add_library(sakana::core ALIAS core)
