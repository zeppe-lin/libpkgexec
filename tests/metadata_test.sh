#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build_root=${1:?build root required}
pc=$(find "$build_root" -name libpkgexec.pc -type f -print -quit)
if [ -z "$pc" ]; then
  echo 'metadata-test: libpkgexec.pc not found' >&2
  exit 1
fi
fail()
{
  echo "metadata-test: $1" >&2
  echo '--- generated metadata ---' >&2
  cat "$pc" >&2
  exit 1
}
grep -Eq '^Name:[[:space:]]+libpkgexec$' "$pc" || fail 'wrong module name'
grep -Eq '^Version:[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+$' "$pc" || fail 'missing version'
grep -Eq '^Libs:.*-lpkgexec([[:space:]]|$)' "$pc" || fail 'missing execution library'
grep -Eq '(^|[[:space:],])libpkgsource[[:space:]]*>=[[:space:]]*1\.1\.0([[:space:],]|$)' "$pc" ||
  fail 'missing exact source authority floor'
