project(
  'wind',
  'cpp',
  version: '0.1.0',
  meson_version: '>=1.10.0',
  default_options: [
    'cpp_std=c++23',
    'build.cpp_std=c++23',
    'warning_level=3',
    'werror=false',
    'buildtype=debugoptimized',
  ],
  license: 'MIT',
)

cpp = meson.get_compiler('cpp')

sdl3_project = subproject('sdl3')
sdl3 = sdl3_project.get_variable('sdl3_dep')

spdlog_project = subproject('spdlog')
spdlog = spdlog_project.get_variable('spdlog_dep')

vulkan = dependency('vulkan', required: true)

compile_args = ['-Wall', '-Wextra', '-Wpedantic', '-fno-exceptions']

if host_machine.system() == 'windows'
  compile_args += [
    '-DWIN32_LEAN_AND_MEAN',
    '-DNOMINMAX',
  ]
endif

# Meson exposes the active build configuration through get_option('buildtype').
buildtype = get_option('buildtype')

if buildtype == 'debug'
  compile_args += [
    '-DRENDERER_DEBUG',
  ]
elif buildtype == 'release'
  compile_args += [
    '-DRENDERER_RELEASE',
  ]
endif

if get_option('validation')
  compile_args += ['-DRENDERER_VULKAN_VALIDATION']
endif

if get_option('renderer_warnings_as_errors')
  compile_args += ['-Werror']
endif

sources = files(
  'src/main.cc',
)

executable(
  'wind',
  sources,
  dependencies: [sdl3, vulkan, spdlog],
  cpp_args: [
    compile_args,
    '-DVULKAN_HPP_NO_EXCEPTIONS',
    '-DVULKAN_HPP_ASSERT_ON_RESULT(x)=',
    '-DGLM_FORCE_DEPTH_ZERO_TO_ONE',
    '-Wno-gnu-statement-expression-from-macro-expansion',
  ],
  install: false,
)

message('=== Build Configuration ===')
message('COMPILER              : ' + cpp.version())
message('C++ Standard          : c++23')
message('SDL3                  : ' + sdl3.version())
message('SPDLOG                : ' + spdlog.version())
message('Vulkan                : ' + vulkan.version())
