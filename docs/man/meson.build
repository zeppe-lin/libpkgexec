# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

manual_pages = [
  ['libpkgexec.3', '3'],
  ['pkgexec_backend.3', '3'],
  ['pkgexec_control.3', '3'],
  ['pkgexec_profile_codec.3', '3'],
  ['pkgexec_request.3', '3'],
  ['pkgexec_result.3', '3'],
  ['pkgexec_result_codec.3', '3'],
  ['pkgexec_semantics.7', '7'],
]

if not get_option('man_pages').disabled()
  foreach spec : manual_pages
    page = spec[0]
    section = spec[1]
    configure_file(
      input: 'generated/' + page,
      output: page,
      copy: true,
      install: true,
      install_dir: get_option('mandir') / ('man' + section),
      install_tag: 'man',
    )
  endforeach
endif

pandoc = find_program('pandoc', required: false)
if pandoc.found()
  update_man_pages = find_program(
    meson.project_source_root() / 'tools/update-man-pages.sh',
  )
  run_target(
    'update-man-pages',
    command: [update_man_pages, '--write', pandoc.full_path(), meson.project_source_root()],
  )
  run_target(
    'check-man-pages',
    command: [update_man_pages, '--check', pandoc.full_path(), meson.project_source_root()],
  )
endif
