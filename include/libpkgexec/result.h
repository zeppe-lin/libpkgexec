// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file result.h
 *  \brief Sealed native execution evidence.
 */
#pragma once

#include <optional>
#include <string>
#include <vector>

#include <libpkgexec/request.h>

namespace pkgexec {

class backend_capability_profile final {
public:
  [[nodiscard]] static backend_capability_profile seal(
      backend_identity backend,
      std::vector<execution_guarantee> guarantees);
  [[nodiscard]] const backend_identity& backend() const noexcept;
  [[nodiscard]] const std::vector<execution_guarantee>& guarantees() const noexcept;
  [[nodiscard]] bool supports(const execution_request& request) const noexcept;
  [[nodiscard]] const backend_capability_profile_identity& identity() const noexcept;
  friend bool operator==(const backend_capability_profile& lhs,
                         const backend_capability_profile& rhs) noexcept;
  friend bool operator!=(const backend_capability_profile& lhs,
                         const backend_capability_profile& rhs) noexcept;
private:
  backend_capability_profile(backend_identity backend,
                             std::vector<execution_guarantee> guarantees,
                             backend_capability_profile_identity identity);
  backend_identity backend_;
  std::vector<execution_guarantee> guarantees_;
  backend_capability_profile_identity identity_;
};

class execution_result final {
public:
  [[nodiscard]] static execution_result failed_before_start(
      execution_request request,
      backend_capability_profile backend,
      execution_failure_kind failure,
      std::vector<execution_guarantee> established_guarantees = {},
      std::string diagnostic = {});

  [[nodiscard]] static execution_result succeeded(
      execution_request request,
      backend_capability_profile backend,
      interpreter_identity observed_interpreter,
      std::optional<stream_capture> standard_output,
      std::optional<stream_capture> standard_error,
      std::vector<execution_guarantee> established_guarantees,
      std::string diagnostic = {});

  [[nodiscard]] static execution_result failed_after_start(
      execution_request request,
      backend_capability_profile backend,
      interpreter_identity observed_interpreter,
      process_termination termination,
      std::optional<stream_capture> standard_output,
      std::optional<stream_capture> standard_error,
      std::vector<execution_guarantee> established_guarantees,
      cleanup_outcome cleanup,
      execution_failure_kind failure,
      std::string diagnostic = {});

  [[nodiscard]] execution_status status() const noexcept;
  [[nodiscard]] execution_start_state start_state() const noexcept;
  [[nodiscard]] const execution_request& request() const noexcept;
  [[nodiscard]] const backend_capability_profile& backend() const noexcept;
  [[nodiscard]] const std::optional<interpreter_identity>& observed_interpreter() const noexcept;
  [[nodiscard]] const std::optional<process_termination>& termination() const noexcept;
  [[nodiscard]] const std::optional<stream_capture>& standard_output() const noexcept;
  [[nodiscard]] const std::optional<stream_capture>& standard_error() const noexcept;
  [[nodiscard]] const std::vector<execution_guarantee>& established_guarantees() const noexcept;
  [[nodiscard]] cleanup_outcome cleanup() const noexcept;
  [[nodiscard]] const std::optional<execution_failure_kind>& failure() const noexcept;
  [[nodiscard]] const std::string& diagnostic() const noexcept;
  [[nodiscard]] const execution_evidence_identity& identity() const noexcept;

  friend bool operator==(const execution_result& lhs,
                         const execution_result& rhs) noexcept;
  friend bool operator!=(const execution_result& lhs,
                         const execution_result& rhs) noexcept;
private:
  execution_result(execution_status status,
                   execution_start_state start_state,
                   execution_request request,
                   backend_capability_profile backend,
                   std::optional<interpreter_identity> observed_interpreter,
                   std::optional<process_termination> termination,
                   std::optional<stream_capture> standard_output,
                   std::optional<stream_capture> standard_error,
                   std::vector<execution_guarantee> established_guarantees,
                   cleanup_outcome cleanup,
                   std::optional<execution_failure_kind> failure,
                   std::string diagnostic,
                   execution_evidence_identity identity);

  execution_status status_;
  execution_start_state start_state_;
  execution_request request_;
  backend_capability_profile backend_;
  std::optional<interpreter_identity> observed_interpreter_;
  std::optional<process_termination> termination_;
  std::optional<stream_capture> standard_output_;
  std::optional<stream_capture> standard_error_;
  std::vector<execution_guarantee> established_guarantees_;
  cleanup_outcome cleanup_;
  std::optional<execution_failure_kind> failure_;
  std::string diagnostic_;
  execution_evidence_identity identity_;
};

} // namespace pkgexec
