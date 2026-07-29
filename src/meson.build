# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
public_inc = include_directories('../include')
private_inc = include_directories('.')

libpkgexec = library(
  'pkgexec',
  [
    'backend.cpp',
    'control.cpp',
    'error.cpp',
    'identity.cpp',
    'identity_support.cpp',
    'model.cpp',
    'request.cpp',
    'result.cpp',
  ],
  include_directories: [public_inc, private_inc],
  dependencies: [libcrypto_dep, libpkgsource_dep, threads_dep],
  install: true,
  version: meson.project_version(),
  soversion: '1',
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
  '../include/libpkgexec/identity.h',
  '../include/libpkgexec/model.h',
  '../include/libpkgexec/request.h',
  '../include/libpkgexec/result.h',
  subdir: 'libpkgexec',
)

pkgconf.generate(
  libpkgexec,
  name: 'libpkgexec',
  filebase: 'libpkgexec',
  description: 'Native sealed program-execution authority',
  subdirs: '.',
  requires: ['libpkgsource >= 2.0.0'],
  requires_private: ['libcrypto'],
  libraries_private: ['-pthread'],
)
