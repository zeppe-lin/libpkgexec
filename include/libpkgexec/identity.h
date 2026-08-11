// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgexec/export.h>

#include <string>

namespace pkgexec {

#define PKGEXEC_DECLARE_IDENTITY(type_name)                                    \
class PKGEXEC_API type_name final {                                                        \
public:                                                                        \
  [[nodiscard]] static type_name from_sha256(std::string hex);                 \
  [[nodiscard]] const std::string& hex() const noexcept;                       \
  friend PKGEXEC_API bool operator==(const type_name& lhs, const type_name& rhs) noexcept; \
  friend PKGEXEC_API bool operator!=(const type_name& lhs, const type_name& rhs) noexcept; \
  friend PKGEXEC_API bool operator<(const type_name& lhs, const type_name& rhs) noexcept;  \
private:                                                                       \
  explicit type_name(std::string hex);                                         \
  std::string hex_;                                                            \
}

PKGEXEC_DECLARE_IDENTITY(execution_request_identity);
PKGEXEC_DECLARE_IDENTITY(environment_policy_identity);
PKGEXEC_DECLARE_IDENTITY(resource_layout_identity);
PKGEXEC_DECLARE_IDENTITY(credential_policy_identity);
PKGEXEC_DECLARE_IDENTITY(resource_limits_identity);
PKGEXEC_DECLARE_IDENTITY(root_view_identity);
PKGEXEC_DECLARE_IDENTITY(resource_identity);
PKGEXEC_DECLARE_IDENTITY(interpreter_identity);
PKGEXEC_DECLARE_IDENTITY(backend_identity);
PKGEXEC_DECLARE_IDENTITY(backend_capability_profile_identity);
PKGEXEC_DECLARE_IDENTITY(execution_evidence_identity);

#undef PKGEXEC_DECLARE_IDENTITY

} // namespace pkgexec
