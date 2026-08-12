# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
public_inc = include_directories('../include')
private_inc = include_directories('.')

elf_export_generator = find_program(
  meson.project_source_root() / 'tools/generate-elf-export-script.sh',
)
core_export_script_name = 'libpkgexec.exports.map'
core_export_script_path = meson.current_build_dir() / core_export_script_name
core_export_script = configure_file(
  input: '../abi/libpkgexec.exports',
  output: core_export_script_name,
  command: [elf_export_generator, '@INPUT@'],
  capture: true,
)

libpkgexec = library(
  'pkgexec',
  [
    'backend.cpp',
    'control.cpp',
    'error.cpp',
    'identity.cpp',
    'identity_support.cpp',
    'model.cpp',
    'profile_codec.cpp',
    'request.cpp',
    'result.cpp',
    'result_codec.cpp',
  ],
  include_directories: [public_inc, private_inc],
  dependencies: [libcrypto_dep, libpkgsource_dep, threads_dep],
  cpp_args: visibility_cpp_args + ['-DPKGEXEC_BUILDING_LIBRARY'],
  gnu_symbol_visibility: 'hidden',
  link_args: ['-Wl,--version-script=' + core_export_script_path],
  link_depends: [core_export_script],
  install: true,
  version: meson.project_version(),
  soversion: '2',
)

libpkgexec_dep = declare_dependency(
  link_with: libpkgexec,
  include_directories: public_inc,
  dependencies: [libcrypto_dep, libpkgsource_dep, threads_dep],
)
meson.override_dependency('libpkgexec', libpkgexec_dep)

install_headers(
  '../include/libpkgexec/libpkgexec.h',
  '../include/libpkgexec/backend.h',
  '../include/libpkgexec/control.h',
  '../include/libpkgexec/error.h',
  '../include/libpkgexec/export.h',
  '../include/libpkgexec/identity.h',
  '../include/libpkgexec/model.h',
  '../include/libpkgexec/profile_codec.h',
  '../include/libpkgexec/request.h',
  '../include/libpkgexec/result.h',
  '../include/libpkgexec/result_codec.h',
  subdir: 'libpkgexec',
)

pkgconf.generate(
  libpkgexec,
  name: 'libpkgexec',
  filebase: 'libpkgexec',
  description: 'Native sealed program-execution authority',
  subdirs: '.',
  requires: [libpkgsource_dep],
  requires_private: [libcrypto_dep],
  libraries_private: ['-pthread'],
)
