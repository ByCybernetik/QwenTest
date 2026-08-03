project('vulkan_triangle', 'c',
  version: '1.0.0',
  default_options: ['c_std=c11']
)

# Dependencies
sdl3_dep = dependency('sdl3', required: true)
vulkan_dep = dependency('vulkan', required: true)
threads_dep = dependency('threads')

# Main executable
executable('vulkan_triangle',
  'main.c',
  dependencies: [sdl3_dep, vulkan_dep, threads_dep],
  install: true
)
