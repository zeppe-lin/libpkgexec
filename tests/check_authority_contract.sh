#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
compiled=$(find "$root/include" "$root/src" -type f \( -name '*.h' -o -name '*.cpp' \) -print)
forbidden='fork\(|execve\(|clone\(|unshare\(|setns\(|landlock|cgroup|mount\(|Pkgfile|fakeroot|pkgmk|pkgman\.conf'
if grep -En "$forbidden" $compiled; then
  echo 'authority-contract: executor or compatibility behavior leaked into the core' >&2
  exit 1
fi
grep -q 'virtual execution_result execute' "$root/include/libpkgexec/backend.h" || {
  echo 'authority-contract: backend execution boundary missing' >&2
  exit 1
}
grep -q 'pkgsource::program' "$root/include/libpkgexec/request.h" || {
  echo 'authority-contract: request does not retain source-owned program authority' >&2
  exit 1
}
grep -q 'std::filesystem::path host_path' "$root/include/libpkgexec/backend.h" || {
  echo 'authority-contract: call-scoped concrete resource path boundary missing' >&2
  exit 1
}
