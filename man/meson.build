# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
scdoc = find_program('scdoc', required: get_option('man_pages'))
if scdoc.found()
  pages = [
    ['libpkgexec.3.scd', 'libpkgexec.3', 'man3'],
    ['pkgexec_request.3.scd', 'pkgexec_request.3', 'man3'],
    ['pkgexec_result.3.scd', 'pkgexec_result.3', 'man3'],
    ['pkgexec_profile_codec.3.scd', 'pkgexec_profile_codec.3', 'man3'],
    ['pkgexec_result_codec.3.scd', 'pkgexec_result_codec.3', 'man3'],
    ['pkgexec_backend.3.scd', 'pkgexec_backend.3', 'man3'],
    ['pkgexec_control.3.scd', 'pkgexec_control.3', 'man3'],
    ['pkgexec_semantics.7.scd', 'pkgexec_semantics.7', 'man7'],
  ]
  foreach page : pages
    custom_target(
      page[1],
      input: page[0],
      output: page[1],
      command: [scdoc],
      feed: true,
      capture: true,
      install: true,
      install_dir: join_paths(get_option('mandir'), page[2]),
    )
  endforeach
endif
