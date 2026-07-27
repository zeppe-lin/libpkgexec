// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/identity.h>

#include "identity_support.h"

#include <utility>

namespace pkgexec {
#define PKGEXEC_DEFINE_IDENTITY(type_name)                                     \
type_name::type_name(std::string hex) : hex_(std::move(hex)) {}                \
type_name type_name::from_sha256(std::string hex)                              \
{                                                                              \
  detail::require_sha256_hex(hex);                                             \
  return type_name(std::move(hex));                                            \
}                                                                              \
const std::string& type_name::hex() const noexcept { return hex_; }             \
bool operator==(const type_name& lhs, const type_name& rhs) noexcept           \
{ return lhs.hex_ == rhs.hex_; }                                               \
bool operator!=(const type_name& lhs, const type_name& rhs) noexcept           \
{ return !(lhs == rhs); }                                                      \
bool operator<(const type_name& lhs, const type_name& rhs) noexcept            \
{ return lhs.hex_ < rhs.hex_; }

PKGEXEC_DEFINE_IDENTITY(execution_request_identity)
PKGEXEC_DEFINE_IDENTITY(environment_policy_identity)
PKGEXEC_DEFINE_IDENTITY(resource_layout_identity)
PKGEXEC_DEFINE_IDENTITY(credential_policy_identity)
PKGEXEC_DEFINE_IDENTITY(resource_limits_identity)
PKGEXEC_DEFINE_IDENTITY(root_view_identity)
PKGEXEC_DEFINE_IDENTITY(resource_identity)
PKGEXEC_DEFINE_IDENTITY(interpreter_identity)
PKGEXEC_DEFINE_IDENTITY(backend_identity)
PKGEXEC_DEFINE_IDENTITY(backend_capability_profile_identity)
PKGEXEC_DEFINE_IDENTITY(execution_evidence_identity)
#undef PKGEXEC_DEFINE_IDENTITY
} // namespace pkgexec
