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

grep -q 'class controlled_execution_backend' "$root/include/libpkgexec/backend.h" || {
  echo 'authority-contract: controlled backend boundary missing' >&2
  exit 1
}
grep -q 'require_cancellation_control(request, cancellation)' "$root/src/backend.cpp" || {
  echo 'authority-contract: controlled dispatch does not admit request-bound control' >&2
  exit 1
}
grep -q 'std::atomic<bool> requested' "$root/src/control.cpp" || {
  echo 'authority-contract: cancellation state is not monotonic call-scoped state' >&2
  exit 1
}

for guarantee in \
  cpu_time_limit address_space_limit file_size_limit open_files_limit process_count_limit
do
  grep -q "execution_guarantee::$guarantee" "$root/src/request.cpp" || {
    echo "authority-contract: request does not derive $guarantee" >&2
    exit 1
  }
done
grep -q 'invalid_capability_profile' "$root/src/result.cpp" || {
  echo 'authority-contract: malformed limit capability profiles are not rejected' >&2
  exit 1
}
grep -q 'resource-limit termination was not requested' "$root/src/result.cpp" || {
  echo 'authority-contract: limit termination is not bound to the request' >&2
  exit 1
}
grep -q 'guarantee_for(\*termination.limit())' "$root/src/result.cpp" || {
  echo 'authority-contract: limit termination lacks exact guarantee admission' >&2
  exit 1
}
grep -q 'execution result claims an unrequested resource limit' "$root/src/result.cpp" || {
  echo 'authority-contract: result limits are not confined to the request' >&2
  exit 1
}
