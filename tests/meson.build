# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').enabled()
  foreach case : [
    ['identity', 'unit/identity_test.cpp'],
    ['purpose-path', 'unit/purpose_path_test.cpp'],
    ['resource-model', 'unit/resource_model_test.cpp'],
    ['environment', 'unit/environment_test.cpp'],
    ['credential-limits', 'unit/credential_limits_test.cpp'],
    ['control', 'unit/control_test.cpp'],
    ['capability-profile', 'unit/capability_profile_test.cpp'],
  ]
    target = executable(
      case[0] + '-test',
      case[1],
      dependencies: libpkgexec_dep,
      install: false,
    )
    test(case[0], target, suite: 'unit')
  endforeach

  foreach case : [
    ['request-sealing', 'integration/request_sealing_test.cpp'],
    ['resource-admission', 'integration/resource_admission_test.cpp'],
    ['backend-dispatch', 'integration/backend_dispatch_test.cpp'],
    ['result-success', 'integration/result_success_test.cpp'],
    ['result-prestart', 'integration/result_prestart_test.cpp'],
    ['result-started', 'integration/result_started_test.cpp'],
    ['result-cancellation', 'integration/result_cancellation_test.cpp'],
    ['result-guarantee-exactness', 'integration/result_guarantee_exactness_test.cpp'],
  ]
    target = executable(
      case[0] + '-test',
      case[1],
      dependencies: libpkgexec_dep,
      install: false,
    )
    test(case[0], target, suite: 'integration')
  endforeach

  foreach case : [
    ['result-codec-roundtrip', 'protocol/result_codec_roundtrip_test.cpp'],
    ['result-codec-corruption', 'protocol/result_codec_corruption_test.cpp'],
  ]
    target = executable(
      case[0] + '-test',
      case[1],
      dependencies: libpkgexec_dep,
      install: false,
    )
    test(case[0], target, suite: 'protocol')
  endforeach

  exec_headers = [
    'backend.h',
    'control.h',
    'error.h',
    'export.h',
    'identity.h',
    'model.h',
    'request.h',
    'result.h',
    'result_codec.h',
  ]
  foreach header : exec_headers
    target = executable(
      'public-header-' + header.underscorify(),
      'header/public_header_test.cpp',
      cpp_args: ['-DPKGEXEC_TEST_HEADER="libpkgexec/' + header + '"'],
      dependencies: libpkgexec_dep,
      install: false,
    )
    test('public-header-' + header, target, suite: 'header')
  endforeach
  umbrella = executable(
    'umbrella-header-test',
    'header/umbrella_header_test.cpp',
    dependencies: libpkgexec_dep,
    install: false,
  )
  test('umbrella-header', umbrella, suite: 'header')

  abi_layout = executable(
    'abi-layout-test',
    'contracts/abi_layout_test.cpp',
    dependencies: libpkgexec_dep,
    install: false,
  )
  test('abi-layout', abi_layout, suite: 'contract')

  sh = find_program('sh')
  if library_kind == 'shared'
    nm = find_program('nm')
    test(
      'abi-surface',
      sh,
      args: [
        meson.current_source_dir() / 'contracts/check_abi_surface.sh',
        nm.full_path(),
        libpkgexec.full_path(),
        meson.project_source_root() / 'abi/libpkgexec.exports',
      ],
      suite: 'contract',
    )
    test(
      'dependency-abi',
      sh,
      args: [
        meson.current_source_dir() / 'contracts/check_dependency_abi.sh',
        libpkgexec.full_path(),
      ],
      suite: 'contract',
    )
  endif

  foreach contract : [
    'pkgconfig_metadata',
    'release_metadata',
    'authority_contract',
    'result_codec_contract',
    'test_layout',
    'abi_contract',
    'ci_contract',
  ]
    script = join_paths(
      meson.current_source_dir(),
      'contracts/check_' + contract + '.sh',
    )
    args = [meson.project_source_root()]
    if contract == 'pkgconfig_metadata'
      args = [meson.project_build_root()]
    endif
    test(
      contract.replace('_', '-'),
      sh,
      args: [script] + args,
      suite: 'contract',
    )
  endforeach
endif
