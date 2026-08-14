#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail() { echo "release-metadata: $*" >&2; exit 1; }
grep -F "version: '2.1.1'" "$root/meson.build" >/dev/null || fail 'project version is not 2.1.1'
grep -F "soversion: '2'" "$root/src/meson.build" >/dev/null || fail 'SONAME is not 2'
block=$(sed -n '/^libpkgsource_dep = dependency(/,/^)/p' "$root/meson.build")
printf '%s\n' "$block" | grep -F "  version: ['>=4.0.0', '<5.0.0']," >/dev/null ||
  fail 'source dependency interval is not >=4.0.0,<5.0.0'
! printf '%s\n' "$block" | grep -F 'fallback:' >/dev/null || fail 'source fallback remains in generation-2 release'
for ghost in planner_adapter yaml_adapter; do
  ! grep -F "$ghost" "$root/meson.build" >/dev/null || fail "obsolete source adapter option remains: $ghost"
done
[ "$(grep -Fxc '  requires: [libpkgsource_dep],' "$root/src/meson.build" || true)" -eq 1 ] ||
  fail 'pkg-config source dependency object must occur exactly once'
[ "$(grep -Fxc '  requires_private: [libcrypto_dep],' "$root/src/meson.build" || true)" -eq 1 ] ||
  fail 'pkg-config private crypto dependency must occur exactly once'
grep -F '## libpkgexec 2.1.1' "$root/HISTORY.md" >/dev/null || fail '2.1.1 history entry is absent'
