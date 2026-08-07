project(
  'tower',
  'cpp',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++23'],
)

add_project_arguments(
  ['-Wconversion', '-g'],
  language: 'cpp',
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
    sdl_image_proj.dependency('SDL3_image-shared')
]

sources = [
    'main.cpp',
]

exe = executable(
  'tower',
  sources,
  install : true,
  dependencies: dependencies,
)

test('basic', exe)
