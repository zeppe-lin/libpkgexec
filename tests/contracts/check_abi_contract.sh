#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
fail() { echo "abi-contract: $*" >&2; exit 1; }
manifest=$root/abi/libpkgexec.exports
[ -s "$manifest" ] || fail 'reviewed ELF ABI manifest is absent'
count=$(sed -n '/^_Z[A-Za-z0-9_]*$/p' "$manifest" | wc -l)
[ "$count" -eq 268 ] || fail "reviewed ELF ABI manifest contains $count symbols, expected 268"
[ "$(LC_ALL=C sort -u "$manifest" | wc -l)" -eq 268 ] ||
  fail 'reviewed ELF ABI manifest contains duplicate symbols'
! grep -E '^_ZNSt|^_ZN9__gnu_cxx|^_ZSt' "$manifest" >/dev/null ||
  fail 'standard-library implementation symbol entered public ABI manifest'
demangled=$(mktemp)
trap 'rm -f "$demangled"' EXIT HUP INT TERM
c++filt < "$manifest" > "$demangled"
! grep -F 'pkgexec::detail::' "$demangled" >/dev/null ||
  fail 'private detail namespace entered public ABI manifest'
for class in \
  execution_purpose logical_path resource_slot resource_layout \
  environment_policy credential_policy resource_limits cancellation_policy \
  process_termination stream_capture execution_request \
  backend_capability_profile execution_result cancellation_token \
  cancellation_source execution_resources
 do
  ! grep -F "pkgexec::$class::$class(" "$demangled" >/dev/null ||
    fail "private $class constructor entered public ABI manifest"
done
for identity in \
  execution_request_identity environment_policy_identity resource_layout_identity \
  credential_policy_identity resource_limits_identity root_view_identity \
  resource_identity interpreter_identity backend_identity \
  backend_capability_profile_identity execution_evidence_identity
 do
  ! grep -F "pkgexec::$identity::$identity(" "$demangled" >/dev/null ||
    fail "private $identity constructor entered public ABI manifest"
done
! grep -F 'execution_result::failed_after_start_impl' "$demangled" >/dev/null ||
  fail 'private result helper entered public ABI manifest'
for required in \
  '_ZTIN7pkgexec5errorE' \
  '_ZTVN7pkgexec17execution_backendE' \
  '_ZTVN7pkgexec28controlled_execution_backendE'
 do
  grep -Fx "$required" "$manifest" >/dev/null ||
    fail "required public ABI symbol is absent: $required"
done
grep -F "soversion: '2'" "$root/src/meson.build" >/dev/null ||
  fail 'SONAME generation is not 2'
grep -F -- '--version-script=' "$root/src/meson.build" >/dev/null ||
  fail 'reviewed ELF export manifest is not linked'
grep -F '../abi/libpkgexec.exports' "$root/src/meson.build" >/dev/null ||
  fail 'Meson does not consume reviewed ABI manifest'
