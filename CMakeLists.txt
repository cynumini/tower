cmake_minimum_required(VERSION 3.25)

project(tower CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# compile options
set(COMPILE_OPTIONS -Wall -Wextra -Wpedantic -fno-exceptions -Wno-c2y-extensions)

# shader_header_gen
add_executable(shader_header_gen tools/shader_header_gen.cpp)
target_compile_options(shader_header_gen PRIVATE ${COMPILE_OPTIONS})

# shaders.hpp
set(SHADERS ui.vert ui.frag world.vert world.frag)
set(SHADER_OUTPUTS)
set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/shaders.hpp")

foreach(SHADER IN LISTS SHADERS)
  set(INPUT "${CMAKE_SOURCE_DIR}/${SHADER}.hlsl")
  set(SPV "${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.spv")
  set(DXIL "${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.dxil")
  add_custom_command(OUTPUT ${SPV}
    COMMAND shadercross "${INPUT}" -o "${SPV}" DEPENDS "${INPUT}" VERBATIM)
  add_custom_command(OUTPUT ${DXIL}
    COMMAND shadercross "${INPUT}" -o "${DXIL}" DEPENDS "${INPUT}" VERBATIM)
  list(APPEND SHADER_OUTPUTS "${SPV}" "${DXIL}")
endforeach()

add_custom_command(OUTPUT "${SHADER_HEADER}"
  COMMAND shader_header_gen ${SHADER_OUTPUTS} "${SHADER_HEADER}"
  DEPENDS ${SHADER_OUTPUTS} shader_header_gen)

add_custom_target(compile_shaders DEPENDS "${SHADER_HEADER}")

# tower
find_package(SDL3 REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(tower src/main.cpp)

target_include_directories(tower PRIVATE include sakana "${CMAKE_CURRENT_BINARY_DIR}")

add_dependencies(tower compile_shaders)

target_link_libraries(tower PRIVATE SDL3::SDL3 nlohmann_json::nlohmann_json)

target_compile_options(tower PRIVATE ${COMPILE_OPTIONS})
