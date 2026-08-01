# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  foreach test_spec : [
    ['identity', 'identity_test.cpp'],
    ['model', 'model_test.cpp'],
    ['request', 'request_test.cpp'],
    ['result', 'result_test.cpp'],
    ['result-codec', 'result_codec_test.cpp'],
    ['backend', 'backend_test.cpp'],
    ['control', 'control_test.cpp'],
  ]
    test_name = test_spec[0]
    test_source = test_spec[1]
    test_binary = executable(
      test_name + '-test',
      test_source,
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
  test(
    'result-codec-contract',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'check_result_codec_contract.sh'),
      meson.project_source_root(),
    ],
  )
endif
