# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  foreach test_name : [
    'identity',
    'model',
    'request',
    'result',
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

endif
