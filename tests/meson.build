# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  foreach test_name : [
    'identity',
    'model',
    'request',
    'result',
    'backend',
  ]
    test_binary = executable(
      test_name + '-test',
      test_name + '_test.cpp',
      include_directories: include_directories('.'),
      dependencies: libpkgexec_dep,
      install: false,
    )
    test(test_name, test_binary)
  endforeach

  public_headers = executable(
    'public-headers',
    'public_headers.cpp',
    dependencies: libpkgexec_dep,
    install: false,
  )
  test('public-headers', public_headers)

  sh = find_program('sh')
  test(
    'metadata',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'metadata_test.sh'),
      meson.project_build_root(),
    ],
  )
  test(
    'release-metadata',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'check_release_metadata.sh'),
      meson.project_source_root(),
    ],
  )
  test(
    'authority-contract',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'check_authority_contract.sh'),
      meson.project_source_root(),
    ],
  )
endif
