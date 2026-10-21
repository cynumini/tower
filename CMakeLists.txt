cmake_minimum_required(VERSION 3.10)

project(tower CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED CONFIG)

add_executable(tower src/main.cpp)
target_compile_options(tower PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions -Wno-c2y-extensions)

add_executable(shader_header_gen tools/shader_header_gen.cpp)
target_compile_options(shader_header_gen PRIVATE -Wall -Wextra -Wpedantic -fno-exceptions)

target_link_libraries(tower PRIVATE SDL3::SDL3)

set(SHADERS ui.vert ui.frag world.vert world.frag)
set(SHADER_OUTPUTS)

foreach(SHADER IN LISTS SHADERS)
  set(INPUT "${CMAKE_SOURCE_DIR}/${SHADER}.hlsl")
  set(SPV "${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.spv")
  set(DXIL "${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.dxil")
  add_custom_command(OUTPUT ${SPV} COMMAND shadercross "${INPUT}" -o "${SPV}")
  add_custom_command(OUTPUT ${DXIL} COMMAND shadercross "${INPUT}" -o "${DXIL}")
  list(APPEND SHADER_OUTPUTS "${SPV}" "${DXIL}")
endforeach()

set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp")

add_custom_command(OUTPUT "${SHADER_HEADER}"
  COMMAND shader_header_gen ${SHADER_OUTPUTS} "${SHADER_HEADER}"
  DEPENDS ${SHADER_OUTPUTS} shader_header_gen)

add_custom_target(compile_shaders DEPENDS "${SHADER_HEADER}")

target_include_directories(tower PRIVATE include sakana)
add_dependencies(tower compile_shaders)
