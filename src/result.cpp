// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/result.h>

#include <libpkgexec/error.h>

#include "identity_support.h"

#include <algorithm>
#include <utility>

namespace pkgexec {
namespace {

template <typename Enum>
constexpr std::uint8_t enum_byte(Enum value) noexcept
{
  return static_cast<std::uint8_t>(value);
}

bool includes(const std::vector<execution_guarantee>& superset,
              const std::vector<execution_guarantee>& subset) noexcept
{
  return std::includes(superset.begin(), superset.end(),
                       subset.begin(), subset.end());
}

std::vector<execution_guarantee> normalize_guarantees(
    std::vector<execution_guarantee> values)
{
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

bool has_guarantee(const std::vector<execution_guarantee>& values,
                   execution_guarantee value) noexcept
{
  return std::binary_search(values.begin(), values.end(), value);
}

backend_capability_profile_identity identify_profile(
    const backend_identity& backend,
    const std::vector<execution_guarantee>& guarantees)
{
  detail::identity_builder hash("pkgexec/backend-capability-profile/v1");
  hash.add_string(backend.hex());
  hash.add_u64(guarantees.size());
  for (const auto guarantee : guarantees) {
    hash.add_u8(enum_byte(guarantee));
  }
  return backend_capability_profile_identity::from_sha256(hash.finish());
}

void append_capture(detail::identity_builder& hash,
                    const std::optional<stream_capture>& capture)
{
  hash.add_bool(capture.has_value());
  if (capture) {
    hash.add_u64(capture->byte_count());
    hash.add_string(capture->digest().hex());
  }
}

execution_evidence_identity identify_result(
    execution_status status,
    execution_start_state start_state,
    const execution_request& request,
    const backend_capability_profile& backend,
    const std::optional<interpreter_identity>& observed_interpreter,
    const std::optional<process_termination>& termination,
    const std::optional<stream_capture>& standard_output,
    const std::optional<stream_capture>& standard_error,
    const std::vector<execution_guarantee>& established_guarantees,
    cleanup_outcome cleanup,
    const std::optional<execution_failure_kind>& failure)
{
  detail::identity_builder hash("pkgexec/execution-evidence/v1");
  hash.add_u8(enum_byte(status));
  hash.add_u8(enum_byte(start_state));
  hash.add_string(request.identity().hex());
  hash.add_string(backend.identity().hex());
  hash.add_bool(observed_interpreter.has_value());
  if (observed_interpreter) {
    hash.add_string(observed_interpreter->hex());
  }
  hash.add_bool(termination.has_value());
  if (termination) {
    hash.add_u8(enum_byte(termination->kind()));
    hash.add_bool(termination->value().has_value());
    if (termination->value()) {
      hash.add_u32(*termination->value());
    }
    hash.add_bool(termination->limit().has_value());
    if (termination->limit()) {
      hash.add_u8(enum_byte(*termination->limit()));
    }
  }
  append_capture(hash, standard_output);
  append_capture(hash, standard_error);
  hash.add_u64(established_guarantees.size());
  for (const auto guarantee : established_guarantees) {
    hash.add_u8(enum_byte(guarantee));
  }
  hash.add_u8(enum_byte(cleanup));
  hash.add_bool(failure.has_value());
  if (failure) {
    hash.add_u8(enum_byte(*failure));
  }
  return execution_evidence_identity::from_sha256(hash.finish());
}

bool pre_start_failure(execution_failure_kind failure) noexcept
{
  switch (failure) {
    case execution_failure_kind::request_rejected:
    case execution_failure_kind::backend_unsupported:
    case execution_failure_kind::resource_admission_failed:
    case execution_failure_kind::interpreter_unavailable:
    case execution_failure_kind::isolation_setup_failed:
    case execution_failure_kind::process_start_failed:
      return true;
    case execution_failure_kind::program_exited_nonzero:
    case execution_failure_kind::program_terminated_by_signal:
    case execution_failure_kind::resource_limit_exceeded:
    case execution_failure_kind::cancelled:
    case execution_failure_kind::log_capture_failed:
    case execution_failure_kind::cleanup_failed:
      return false;
  }
  return false;
}

bool before_start_guarantee(execution_guarantee guarantee) noexcept
{
  return guarantee != execution_guarantee::complete_stdout_capture &&
         guarantee != execution_guarantee::complete_stderr_capture &&
         guarantee != execution_guarantee::cleanup_verified;
}

std::vector<execution_guarantee> before_start_requirements(
    const execution_request& request)
{
  std::vector<execution_guarantee> result;
  for (const auto guarantee : request.required_guarantees()) {
    if (before_start_guarantee(guarantee)) {
      result.push_back(guarantee);
    }
  }
  return result;
}

void require_capture_shape(
    const execution_request& request,
    const std::optional<stream_capture>& standard_output,
    const std::optional<stream_capture>& standard_error,
    bool allow_missing)
{
  const bool want_stdout =
      request.environment().standard_output() == stream_policy::capture_complete;
  const bool want_stderr =
      request.environment().standard_error() == stream_policy::capture_complete;
  if ((!allow_missing && want_stdout != standard_output.has_value()) ||
      (!allow_missing && want_stderr != standard_error.has_value()) ||
      (!want_stdout && standard_output.has_value()) ||
      (!want_stderr && standard_error.has_value())) {
    throw error(error_code::inconsistent_result,
                "captured streams do not match the request I/O policy");
  }
}

void require_subset(const std::vector<execution_guarantee>& established,
                    const backend_capability_profile& backend)
{
  if (!includes(backend.guarantees(), established)) {
    throw error(error_code::inconsistent_result,
                "execution result claims a guarantee absent from the backend profile");
  }
}

void require_requested_control(const execution_request& request,
                               const cancellation_token& cancellation)
{
  require_cancellation_control(request, cancellation);
  if (!cancellation.cancellation_requested()) {
    throw error(error_code::invalid_control,
                "cancellation evidence requires a requested cancellation signal");
  }
}

} // namespace

backend_capability_profile::backend_capability_profile(
    backend_identity backend,
    std::vector<execution_guarantee> guarantees,
    backend_capability_profile_identity identity)
    : backend_(std::move(backend)), guarantees_(std::move(guarantees)),
      identity_(std::move(identity))
{
}
backend_capability_profile backend_capability_profile::seal(
    backend_identity backend,
    std::vector<execution_guarantee> guarantees)
{
  guarantees = normalize_guarantees(std::move(guarantees));
  auto identity = identify_profile(backend, guarantees);
  return backend_capability_profile(std::move(backend), std::move(guarantees),
                                    std::move(identity));
}
const backend_identity& backend_capability_profile::backend() const noexcept
{ return backend_; }
const std::vector<execution_guarantee>& backend_capability_profile::guarantees() const noexcept
{ return guarantees_; }
bool backend_capability_profile::supports(const execution_request& request) const noexcept
{ return includes(guarantees_, request.required_guarantees()); }
const backend_capability_profile_identity& backend_capability_profile::identity() const noexcept
{ return identity_; }
bool operator==(const backend_capability_profile& lhs,
                const backend_capability_profile& rhs) noexcept
{ return lhs.identity_ == rhs.identity_; }
bool operator!=(const backend_capability_profile& lhs,
                const backend_capability_profile& rhs) noexcept
{ return !(lhs == rhs); }

execution_result::execution_result(
    execution_status status,
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
    execution_evidence_identity identity)
    : status_(status), start_state_(start_state), request_(std::move(request)),
      backend_(std::move(backend)),
      observed_interpreter_(std::move(observed_interpreter)),
      termination_(std::move(termination)),
      standard_output_(std::move(standard_output)),
      standard_error_(std::move(standard_error)),
      established_guarantees_(std::move(established_guarantees)),
      cleanup_(cleanup), failure_(failure), diagnostic_(std::move(diagnostic)),
      identity_(std::move(identity))
{
}

execution_result execution_result::failed_before_start(
    execution_request request,
    backend_capability_profile backend,
    execution_failure_kind failure,
    std::vector<execution_guarantee> established_guarantees,
    std::string diagnostic)
{
  if (failure == execution_failure_kind::cancelled) {
    throw error(error_code::invalid_control,
                "pre-start cancellation requires call-scoped control evidence");
  }
  if (!pre_start_failure(failure)) {
    throw error(error_code::invalid_failure,
                "selected failure kind requires a started process");
  }
  established_guarantees = normalize_guarantees(std::move(established_guarantees));
  require_subset(established_guarantees, backend);
  if (std::any_of(established_guarantees.begin(), established_guarantees.end(),
                  [](execution_guarantee value) {
                    return !before_start_guarantee(value);
                  })) {
    throw error(error_code::inconsistent_result,
                "not-started execution cannot establish capture or cleanup guarantees");
  }
  if (failure != execution_failure_kind::backend_unsupported &&
      !backend.supports(request)) {
    throw error(error_code::unsupported_request,
                "backend profile does not support the rejected request");
  }
  auto identity = identify_result(
      execution_status::failed, execution_start_state::not_started, request,
      backend, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      established_guarantees, cleanup_outcome::not_required, failure);
  return execution_result(
      execution_status::failed, execution_start_state::not_started,
      std::move(request), std::move(backend), std::nullopt, std::nullopt,
      std::nullopt, std::nullopt, std::move(established_guarantees),
      cleanup_outcome::not_required, failure, std::move(diagnostic),
      std::move(identity));
}

execution_result execution_result::cancelled_before_start(
    execution_request request,
    backend_capability_profile backend,
    const cancellation_token& cancellation,
    std::vector<execution_guarantee> established_guarantees,
    std::string diagnostic)
{
  require_requested_control(request, cancellation);
  established_guarantees = normalize_guarantees(std::move(established_guarantees));
  require_subset(established_guarantees, backend);
  if (!backend.supports(request)) {
    throw error(error_code::unsupported_request,
                "backend profile does not support the cancelled request");
  }
  if (!has_guarantee(established_guarantees,
                     execution_guarantee::cancellation)) {
    throw error(error_code::inconsistent_result,
                "cancelled execution must establish cancellation control");
  }
  if (std::any_of(established_guarantees.begin(), established_guarantees.end(),
                  [](execution_guarantee value) {
                    return !before_start_guarantee(value);
                  })) {
    throw error(error_code::inconsistent_result,
                "not-started cancellation cannot establish capture or cleanup guarantees");
  }
  const auto failure = execution_failure_kind::cancelled;
  auto identity = identify_result(
      execution_status::failed, execution_start_state::not_started, request,
      backend, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
      established_guarantees, cleanup_outcome::not_required, failure);
  return execution_result(
      execution_status::failed, execution_start_state::not_started,
      std::move(request), std::move(backend), std::nullopt, std::nullopt,
      std::nullopt, std::nullopt, std::move(established_guarantees),
      cleanup_outcome::not_required, failure, std::move(diagnostic),
      std::move(identity));
}

execution_result execution_result::succeeded(
    execution_request request,
    backend_capability_profile backend,
    interpreter_identity observed_interpreter,
    std::optional<stream_capture> standard_output,
    std::optional<stream_capture> standard_error,
    std::vector<execution_guarantee> established_guarantees,
    std::string diagnostic)
{
  established_guarantees = normalize_guarantees(std::move(established_guarantees));
  if (!backend.supports(request) ||
      !includes(established_guarantees, request.required_guarantees())) {
    throw error(error_code::unsupported_request,
                "successful execution evidence lacks a required guarantee");
  }
  require_subset(established_guarantees, backend);
  if (observed_interpreter != request.interpreter()) {
    throw error(error_code::inconsistent_result,
                "observed interpreter does not match the sealed request");
  }
  require_capture_shape(request, standard_output, standard_error, false);
  auto termination = process_termination::exited(0);
  auto identity = identify_result(
      execution_status::succeeded, execution_start_state::started, request,
      backend, observed_interpreter, termination, standard_output,
      standard_error, established_guarantees, cleanup_outcome::verified,
      std::nullopt);
  return execution_result(
      execution_status::succeeded, execution_start_state::started,
      std::move(request), std::move(backend), std::move(observed_interpreter),
      std::move(termination), std::move(standard_output),
      std::move(standard_error), std::move(established_guarantees),
      cleanup_outcome::verified, std::nullopt, std::move(diagnostic),
      std::move(identity));
}

execution_result execution_result::failed_after_start(
    execution_request request,
    backend_capability_profile backend,
    interpreter_identity observed_interpreter,
    process_termination termination,
    std::optional<stream_capture> standard_output,
    std::optional<stream_capture> standard_error,
    std::vector<execution_guarantee> established_guarantees,
    cleanup_outcome cleanup,
    execution_failure_kind failure,
    std::string diagnostic)
{
  return failed_after_start_impl(
      std::move(request), std::move(backend), std::move(observed_interpreter),
      std::move(termination), std::move(standard_output),
      std::move(standard_error), std::move(established_guarantees), cleanup,
      failure, false, std::move(diagnostic));
}

execution_result execution_result::cancelled_after_start(
    execution_request request,
    backend_capability_profile backend,
    const cancellation_token& cancellation,
    interpreter_identity observed_interpreter,
    std::optional<stream_capture> standard_output,
    std::optional<stream_capture> standard_error,
    std::vector<execution_guarantee> established_guarantees,
    cleanup_outcome cleanup,
    std::string diagnostic)
{
  require_requested_control(request, cancellation);
  return failed_after_start_impl(
      std::move(request), std::move(backend), std::move(observed_interpreter),
      process_termination::cancelled(), std::move(standard_output),
      std::move(standard_error), std::move(established_guarantees), cleanup,
      execution_failure_kind::cancelled, true, std::move(diagnostic));
}

execution_result execution_result::failed_after_start_impl(
    execution_request request,
    backend_capability_profile backend,
    interpreter_identity observed_interpreter,
    process_termination termination,
    std::optional<stream_capture> standard_output,
    std::optional<stream_capture> standard_error,
    std::vector<execution_guarantee> established_guarantees,
    cleanup_outcome cleanup,
    execution_failure_kind failure,
    bool cancellation_admitted,
    std::string diagnostic)
{
  if (pre_start_failure(failure)) {
    throw error(error_code::invalid_failure,
                "selected failure kind requires a not-started result");
  }
  if (cleanup == cleanup_outcome::not_required) {
    throw error(error_code::inconsistent_result,
                "started execution must report cleanup verification");
  }
  if (observed_interpreter != request.interpreter()) {
    throw error(error_code::inconsistent_result,
                "observed interpreter does not match the sealed request");
  }
  established_guarantees = normalize_guarantees(std::move(established_guarantees));
  require_subset(established_guarantees, backend);
  if (!backend.supports(request)) {
    throw error(error_code::unsupported_request,
                "a backend must not start an unsupported request");
  }
  if (!includes(established_guarantees, before_start_requirements(request))) {
    throw error(error_code::inconsistent_result,
                "started execution lacks a required pre-start guarantee");
  }
  const bool stdout_established = has_guarantee(
      established_guarantees,
      execution_guarantee::complete_stdout_capture);
  const bool stderr_established = has_guarantee(
      established_guarantees,
      execution_guarantee::complete_stderr_capture);
  const bool cleanup_established = has_guarantee(
      established_guarantees,
      execution_guarantee::cleanup_verified);
  if (stdout_established != standard_output.has_value() ||
      stderr_established != standard_error.has_value()) {
    throw error(error_code::inconsistent_result,
                "stream evidence and established capture guarantees disagree");
  }
  if ((cleanup == cleanup_outcome::verified) != cleanup_established) {
    throw error(error_code::inconsistent_result,
                "cleanup outcome and established cleanup guarantee disagree");
  }

  switch (failure) {
    case execution_failure_kind::program_exited_nonzero:
      if (termination.kind() != process_termination_kind::exited ||
          !termination.value() || *termination.value() == 0U) {
        throw error(error_code::invalid_failure,
                    "nonzero-exit failure requires a nonzero exit status");
      }
      break;
    case execution_failure_kind::program_terminated_by_signal:
      if (termination.kind() != process_termination_kind::signaled) {
        throw error(error_code::invalid_failure,
                    "signal failure requires signal termination evidence");
      }
      break;
    case execution_failure_kind::resource_limit_exceeded:
      if (termination.kind() != process_termination_kind::resource_limited) {
        throw error(error_code::invalid_failure,
                    "resource-limit failure requires limit termination evidence");
      }
      break;
    case execution_failure_kind::cancelled:
      if (!cancellation_admitted) {
        throw error(error_code::invalid_control,
                    "started cancellation requires call-scoped control evidence");
      }
      if (termination.kind() != process_termination_kind::cancelled) {
        throw error(error_code::invalid_failure,
                    "cancellation failure requires cancellation evidence");
      }
      break;
    case execution_failure_kind::log_capture_failed:
      require_capture_shape(request, standard_output, standard_error, true);
      if (request.environment().standard_output() == stream_policy::capture_complete &&
          standard_output &&
          request.environment().standard_error() == stream_policy::capture_complete &&
          standard_error) {
        throw error(error_code::invalid_failure,
                    "log-capture failure requires missing requested stream evidence");
      }
      break;
    case execution_failure_kind::cleanup_failed:
      if (cleanup != cleanup_outcome::failed) {
        throw error(error_code::invalid_failure,
                    "cleanup failure requires failed cleanup evidence");
      }
      break;
    case execution_failure_kind::request_rejected:
    case execution_failure_kind::backend_unsupported:
    case execution_failure_kind::resource_admission_failed:
    case execution_failure_kind::interpreter_unavailable:
    case execution_failure_kind::isolation_setup_failed:
    case execution_failure_kind::process_start_failed:
      break;
  }
  if (failure != execution_failure_kind::log_capture_failed) {
    require_capture_shape(request, standard_output, standard_error, false);
  }
  if (failure != execution_failure_kind::cleanup_failed &&
      cleanup != cleanup_outcome::verified) {
    throw error(error_code::inconsistent_result,
                "non-cleanup failures still require verified cleanup");
  }

  auto identity = identify_result(
      execution_status::failed, execution_start_state::started, request,
      backend, observed_interpreter, termination, standard_output,
      standard_error, established_guarantees, cleanup, failure);
  return execution_result(
      execution_status::failed, execution_start_state::started,
      std::move(request), std::move(backend), std::move(observed_interpreter),
      std::move(termination), std::move(standard_output),
      std::move(standard_error), std::move(established_guarantees), cleanup,
      failure, std::move(diagnostic), std::move(identity));
}

execution_status execution_result::status() const noexcept { return status_; }
execution_start_state execution_result::start_state() const noexcept
{ return start_state_; }
const execution_request& execution_result::request() const noexcept
{ return request_; }
const backend_capability_profile& execution_result::backend() const noexcept
{ return backend_; }
const std::optional<interpreter_identity>& execution_result::observed_interpreter() const noexcept
{ return observed_interpreter_; }
const std::optional<process_termination>& execution_result::termination() const noexcept
{ return termination_; }
const std::optional<stream_capture>& execution_result::standard_output() const noexcept
{ return standard_output_; }
const std::optional<stream_capture>& execution_result::standard_error() const noexcept
{ return standard_error_; }
const std::vector<execution_guarantee>& execution_result::established_guarantees() const noexcept
{ return established_guarantees_; }
cleanup_outcome execution_result::cleanup() const noexcept { return cleanup_; }
const std::optional<execution_failure_kind>& execution_result::failure() const noexcept
{ return failure_; }
const std::string& execution_result::diagnostic() const noexcept { return diagnostic_; }
const execution_evidence_identity& execution_result::identity() const noexcept
{ return identity_; }
bool operator==(const execution_result& lhs, const execution_result& rhs) noexcept
{ return lhs.identity_ == rhs.identity_; }
bool operator!=(const execution_result& lhs, const execution_result& rhs) noexcept
{ return !(lhs == rhs); }

} // namespace pkgexec
