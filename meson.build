project('tower', 'c')

sdl3 = dependency('sdl3')
cglm = dependency('cglm')
m = meson.get_compiler('c').find_library('m')

add_project_arguments(
    '-DCGLM_CONFIG_CLIP_CONTROL=CGLM_CLIP_CONTROL_RH_ZO',
    language: 'c',
)

shaders = []

foreach shader : ['world.vert', 'world.frag']
    shaders += custom_target(
        shader,
        input: shader + '.hlsl',
        output: shader + '.spv',
        command: ['shadercross', '@INPUT@', '-o', '@OUTPUT@'],
    )
endforeach

executable('tower', ['main.c', shaders], dependencies: [sdl3, m, cglm])
