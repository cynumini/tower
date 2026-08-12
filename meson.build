project(
  'tower',
  'cpp',
  version : '0.3.0',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++23'],
)

add_project_arguments(
  ['-Wconversion', '-g'],
  language: 'cpp',
)


conf = configuration_data()
conf.set_quoted('PROJECT_VERSION', meson.project_version())

configure_file(
    output: 'version.h',
    configuration: conf
)

cmake = import('cmake')

glslc = find_program('glslc')

frag = custom_target('frag',
                     output : 'shader.spv.frag',
                     input : 'shader.glsl.frag',
                     command : [glslc, '@INPUT@', '-o', '@OUTPUT@'],
                     build_by_default: true)

vert = custom_target('vert',
                     output : 'shader.spv.vert',
                     input : 'shader.glsl.vert',
                     command : [glslc, '@INPUT@', '-o', '@OUTPUT@'],
                     build_by_default: true)

sdl_proj = cmake.subproject('SDL')
sdl_image_proj = cmake.subproject('SDL_image')

dependencies = [
    sdl_proj.dependency('SDL3-shared'),
    sdl_image_proj.dependency('SDL3_image-shared'),
]


include = include_directories([
    './imgui/',
    './imgui/backends/'
])

sources = [
    'main.cpp',
    './imgui/imgui.cpp',
    './imgui/imgui_draw.cpp',
    './imgui/imgui_demo.cpp',
    './imgui/imgui_widgets.cpp',
    './imgui/imgui_tables.cpp',
    './imgui/backends/imgui_impl_sdl3.cpp',
    './imgui/backends/imgui_impl_sdlgpu3.cpp',
]

exe = executable(
  'tower',
  sources,
  install : true,
  dependencies: dependencies,
  include_directories: include,
)

test('basic', exe)
