#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail(){ echo "test-layout-contract: $*" >&2; exit 1; }
meson=$root/tests/meson.build

for directory in unit integration protocol header fixtures support contracts installed; do
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

for product_file in \
  "$root/tests/contracts/abi_layout_test.cpp" \
  "$root/tests/contracts/check_abi_surface.sh" \
  "$root/tests/contracts/check_dependency_abi.sh" \
  "$root/tests/contracts/check_abi_contract.sh" \
  "$root/tests/contracts/check_ci_contract.sh" \
  "$root/tests/installed/consumer.cpp"; do
  [ -s "$product_file" ] || {
    echo "test-layout: missing release-product qualification: $product_file" >&2
    exit 1
  }
done
for registration in \
  "'abi-layout'" \
  "'abi-surface'" \
  "'dependency-abi'" \
  "'abi_contract'" \
  "'ci_contract'"; do
  grep -F "$registration" "$meson" >/dev/null || {
    echo "test-layout: Meson omits release-product qualification: $registration" >&2
    exit 1
  }
done
grep -F 'tests/installed/consumer.cpp' "$root/ci/configure-and-test.sh" >/dev/null || {
  echo 'test-layout: installed consumer is not part of release qualification' >&2
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

for contract in "$root"/tests/contracts/check_*.sh; do
  name=${contract##*/check_}
  name=${name%.sh}
  case $name in
    abi_surface|dependency_abi|pkgconfig_metadata|manpage_generated) continue ;;
  esac
  if ! grep -F "'$name'" "$root/tests/meson.build" >/dev/null &&
     ! grep -F "check_${name}.sh" "$root/tests/meson.build" >/dev/null; then
    fail "unregistered contract: check_${name}.sh"
  fi
done
