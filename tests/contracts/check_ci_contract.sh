#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail() { echo "ci-contract: $*" >&2; exit 1; }
workflow=$root/.github/workflows/ci.yml
runner=$root/ci/configure-and-test.sh
qualify=$root/ci/qualify.sh
[ -s "$workflow" ] || fail 'hosted CI workflow is absent'
[ -x "$runner" ] || fail 'qualification runner is absent or not executable'
[ -x "$qualify" ] || fail 'local qualification entry point is absent or not executable'
for text in 'GCC shared' 'GCC static' 'Clang shared' 'Clang static' 'GCC release' 'address,undefined' 'libpkgsource'; do
  grep -F "$text" "$workflow" >/dev/null || fail "workflow omits $text"
done
count=$(grep -F 'ref: v4.1.0' "$workflow" | wc -l | tr -d ' ')
[ "$count" -eq 2 ] || fail 'workflow does not pin both jobs to libpkgsource v4.1.0 authority'
for text in 'meson install -C "$build/product"' 'tests/installed/consumer.cpp' 'pkg-config --static --libs libpkgexec' 'LD_LIBRARY_PATH='; do
  grep -F "$text" "$runner" >/dev/null || fail "runner omits installed-product gate: $text"
done
grep -F 'encode_backend_capability_profile(profile)' "$root/tests/installed/consumer.cpp" >/dev/null ||
  fail 'installed consumer does not exercise backend-profile owner codec'

for text in 'configure-and-test.sh' 'shared static' 'MESON_SANITIZE=address,undefined' 'LIBPKGSOURCE_SOURCE'; do
  grep -F "$text" "$qualify" >/dev/null || fail "local qualification omits release gate: $text"
done
