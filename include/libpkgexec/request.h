// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file request.h
 *  \brief Sealed native execution requests.
 */
#pragma once

#include <vector>

#include <libpkgexec/model.h>

namespace pkgexec {

inline constexpr std::uint16_t execution_request_schema_version = 1;

class execution_request final {
public:
  [[nodiscard]] static execution_request seal(
      pkgsource::program program,
      execution_purpose purpose,
      interpreter_identity interpreter,
      root_view_identity root_view,
      resource_layout resources,
      environment_policy environment,
      credential_policy credentials,
      resource_limits limits,
      cancellation_policy cancellation,
      std::vector<execution_guarantee> additional_required_guarantees = {});

  [[nodiscard]] std::uint16_t schema_version() const noexcept;
  [[nodiscard]] const pkgsource::program& program() const noexcept;
  [[nodiscard]] const execution_purpose& purpose() const noexcept;
  [[nodiscard]] const interpreter_identity& interpreter() const noexcept;
  [[nodiscard]] const root_view_identity& root_view() const noexcept;
  [[nodiscard]] const resource_layout& resources() const noexcept;
  [[nodiscard]] const environment_policy& environment() const noexcept;
  [[nodiscard]] const credential_policy& credentials() const noexcept;
  [[nodiscard]] const resource_limits& limits() const noexcept;
  [[nodiscard]] const cancellation_policy& cancellation() const noexcept;
  [[nodiscard]] const std::vector<execution_guarantee>& required_guarantees() const noexcept;
  [[nodiscard]] const execution_request_identity& identity() const noexcept;

  friend bool operator==(const execution_request& lhs,
                         const execution_request& rhs) noexcept;
  friend bool operator!=(const execution_request& lhs,
                         const execution_request& rhs) noexcept;
private:
  execution_request(pkgsource::program program,
                    execution_purpose purpose,
                    interpreter_identity interpreter,
                    root_view_identity root_view,
                    resource_layout resources,
                    environment_policy environment,
                    credential_policy credentials,
                    resource_limits limits,
                    cancellation_policy cancellation,
                    std::vector<execution_guarantee> required_guarantees,
                    execution_request_identity identity);

  std::uint16_t schema_version_ = execution_request_schema_version;
  pkgsource::program program_;
  execution_purpose purpose_;
  interpreter_identity interpreter_;
  root_view_identity root_view_;
  resource_layout resources_;
  environment_policy environment_;
  credential_policy credentials_;
  resource_limits limits_;
  cancellation_policy cancellation_;
  std::vector<execution_guarantee> required_guarantees_;
  execution_request_identity identity_;
};

} // namespace pkgexec
