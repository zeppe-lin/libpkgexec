#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
version=$(sed -n "s/^[[:space:]]*version: '\([^']*\)'.*/\1/p" "$root/meson.build" | head -n 1)
[ -n "$version" ] || {
  echo 'release-metadata: project version not found' >&2
  exit 1
}
grep -q "## libpkgexec $version" "$root/HISTORY.md" || {
  echo "release-metadata: HISTORY omits $version" >&2
  exit 1
}
case "$version" in
  0.1.0) grep -q "soversion: '0'" "$root/src/meson.build" ;;
  1.0.0|1.1.0) grep -q "soversion: '1'" "$root/src/meson.build" ;;
  *) echo "release-metadata: unexpected project version $version" >&2; exit 1 ;;
esac
grep -q "libpkgsource >= 1.1.0" "$root/src/meson.build" || {
  echo 'release-metadata: pkg-config source authority floor missing' >&2
  exit 1
}
