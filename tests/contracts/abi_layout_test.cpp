// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/libpkgexec.h>

#include <cstddef>

#if !defined(__x86_64__)
#error "libpkgexec 2 ABI layout qualification is x86-64 specific"
#endif

static_assert(sizeof(void*) == 8);
static_assert(alignof(void*) == 8);
static_assert(sizeof(pkgsource::program) == 80);
static_assert(alignof(pkgsource::program) == 8);
static_assert(sizeof(pkgexec::execution_purpose) == 12);
static_assert(sizeof(pkgexec::resource_slot) == 40);
static_assert(sizeof(pkgexec::resource_binding) == 112);
static_assert(sizeof(pkgexec::resource_layout) == 96);
static_assert(sizeof(pkgexec::environment_policy) == 184);
static_assert(sizeof(pkgexec::credential_policy) == 80);
static_assert(sizeof(pkgexec::resource_limits) == 112);
static_assert(sizeof(pkgexec::cancellation_policy) == 24);
static_assert(sizeof(pkgexec::process_termination) == 20);
static_assert(sizeof(pkgexec::stream_capture) == 80);
static_assert(sizeof(pkgexec::execution_request) == 720);
static_assert(sizeof(pkgexec::backend_capability_profile) == 88);
static_assert(sizeof(pkgexec::resource_materialization) == 72);
static_assert(sizeof(pkgexec::execution_resources) == 96);
static_assert(sizeof(pkgexec::cancellation_token) == 16);
static_assert(sizeof(pkgexec::cancellation_source) == 16);
static_assert(sizeof(pkgexec::execution_result) == 1160);

int main() { return 0; }
