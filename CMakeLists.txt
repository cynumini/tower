cmake_minimum_required(VERSION 3.23)
project(tower)

add_subdirectory(vendor/SDL)
add_subdirectory(vendor/SDL_image)
add_subdirectory(vendor/glm)
add_subdirectory(sakana)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(
  imgui STATIC
  vendor/imgui/imgui.cpp
  vendor/imgui/imgui_draw.cpp
  vendor/imgui/imgui_widgets.cpp
  vendor/imgui/imgui_tables.cpp
  vendor/imgui/backends/imgui_impl_sdlgpu3.cpp
  vendor/imgui/backends/imgui_impl_sdl3.cpp
)
target_include_directories(imgui PUBLIC vendor/imgui vendor/imgui/backends)

add_library(tower STATIC main.cpp base.cpp renderer.cpp font.cpp csv.cpp)
target_link_libraries(tower PUBLIC sakana::core SDL3::SDL3 SDL3_image::SDL3_image imgui glm::glm)
target_compile_features(tower PRIVATE cxx_std_23)
target_compile_options(tower PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

add_executable(tower-a game-a.cpp)
target_link_libraries(tower-a PRIVATE tower)
target_compile_features(tower-a PRIVATE cxx_std_23)
target_compile_options(tower-a PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

add_executable(tower-t game-t.cpp)
target_link_libraries(tower-t PRIVATE tower)
target_compile_features(tower-t PRIVATE cxx_std_23)
target_compile_options(tower-t PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

function(compile_shader in out)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${out}"
    COMMAND glslc "${CMAKE_CURRENT_SOURCE_DIR}/${in}" -o "${CMAKE_CURRENT_BINARY_DIR}/${out}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${in}"
    VERBATIM
  )
endfunction()

compile_shader(shader.glsl.vert shader.spv.vert)
compile_shader(shader.glsl.frag shader.spv.frag)
compile_shader(shader3d.glsl.vert shader3d.spv.vert)
compile_shader(shader3d.glsl.frag shader3d.spv.frag)

add_custom_target(shaders
  DEPENDS
  "${CMAKE_CURRENT_BINARY_DIR}/shader.spv.vert"
  "${CMAKE_CURRENT_BINARY_DIR}/shader.spv.frag"
  "${CMAKE_CURRENT_BINARY_DIR}/shader3d.spv.vert"
  "${CMAKE_CURRENT_BINARY_DIR}/shader3d.spv.frag"
)

add_dependencies(tower-a shaders)
add_dependencies(tower-t shaders)
