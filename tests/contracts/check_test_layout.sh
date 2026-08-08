#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
meson=$root/tests/meson.build

for directory in unit integration protocol header fixtures support contracts; do
  [ -d "$root/tests/$directory" ] || {
    echo "test-layout: missing qualification role: $directory" >&2
    exit 1
  }
done
for escaped in "$root"/tests/*.cpp "$root"/tests/*.h "$root"/tests/check_*.sh; do
  [ ! -e "$escaped" ] || {
    echo "test-layout: uncategorized test source: $escaped" >&2
    exit 1
  }
done
for suite in unit integration protocol header contract; do
  grep -F "suite: '$suite'" "$meson" >/dev/null || {
    echo "test-layout: Meson omits $suite suite" >&2
    exit 1
  }
done
for path in \
  unit/resource_model_test.cpp \
  unit/control_test.cpp \
  integration/request_sealing_test.cpp \
  integration/resource_admission_test.cpp \
  integration/result_guarantee_exactness_test.cpp \
  protocol/result_codec_roundtrip_test.cpp \
  protocol/result_codec_corruption_test.cpp; do
  grep -F "$path" "$meson" "$root/TESTING.md" >/dev/null || {
    echo "test-layout: qualification wiring omits $path" >&2
    exit 1
  }
done
grep -F "'test_layout'" "$meson" >/dev/null || {
  echo 'test-layout: Meson omits test-layout contract' >&2
  exit 1
}
grep -F 'test-layout' "$root/TESTING.md" >/dev/null || {
  echo 'test-layout: TESTING omits test-layout contract' >&2
  exit 1
}
for support_file in \
  "$root/tests/fixtures/execution.h" \
  "$root/tests/support/test.h" \
  "$root/tests/support/guarantees.h" \
  "$root/tests/support/result.h"; do
  [ -s "$support_file" ] || {
    echo "test-layout: missing categorized support material: $support_file" >&2
    exit 1
  }
done
