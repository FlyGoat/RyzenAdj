project('ryzenadj', 'c', 'cpp',
  default_options : ['cpp_std=c++11', 'c_visibilty=hidden', 'cpp_visibilty=hidden']
)

add_project_arguments('-D_LIBRYZENADJ_INTERNAL', language : ['c', 'cpp'])

# Include directories
inc_dirs = include_directories('.')  # Adjust this if your include path is different

# Source directories
lib_srcs = files(
  'lib/nb_smu_ops.c',
  'lib/api.c',
  'lib/cpuid.c'
)

# OS-specific settings
if host_machine.system() == 'windows'
  lib_srcs += files('lib/osdep_win32.cpp')
  win32_dir = join_paths(meson.current_source_dir(), 'win32')
  cxx = meson.get_compiler('cpp')
  os_deps = cxx.find_library('WinRing0x64', dirs: [win32_dir])
elif host_machine.system() == 'linux'
  lib_srcs += files('lib/osdep_linux.c')
  os_deps = dependency('libpci', required : true)
else
  error('Unsupported OS')
endif

cli_srcs = files(
  'argparse.c',
  'main.c',
)
cli_inc_dirs = include_directories('lib')

# Define executable
executable('ryzenadj', [lib_srcs, cli_srcs],
            include_directories: cli_inc_dirs,
            dependencies: os_deps,
            install: true)

# Define library
libryzenadj = library('libryzenadj', lib_srcs,
                      dependencies: os_deps,
                      install: true)
install_headers('lib/ryzenadj.h', subdir : 'ryzenadj')
pkg = import('pkgconfig')
pkg.generate(libryzenadj)
