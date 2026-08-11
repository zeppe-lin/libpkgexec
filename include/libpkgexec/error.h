// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgexec/export.h>

#include <stdexcept>
#include <string>

namespace pkgexec {

enum class error_code {
  invalid_identity,
  invalid_value,
  invalid_purpose,
  invalid_path,
  duplicate_resource,
  duplicate_mount_point,
  missing_working_directory,
  duplicate_environment_variable,
  reserved_environment_variable,
  invalid_policy,
  invalid_capability_profile,
  unsupported_request,
  resource_mismatch,
  inconsistent_result,
  invalid_failure,
  identity_failed,
  invalid_control,
  control_mismatch,
  corrupt_encoding,
  authority_mismatch,
};

class PKGEXEC_API error : public std::runtime_error {
public:
  error(error_code code, std::string message);
  ~error() override;
  [[nodiscard]] error_code code() const noexcept;
private:
  error_code code_;
};

} // namespace pkgexec
