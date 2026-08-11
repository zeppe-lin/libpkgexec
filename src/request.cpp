// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/request.h>

#include <libpkgexec/error.h>

#include "identity_support.h"
#include "vocabulary.h"

#include <algorithm>
#include <set>
#include <utility>

namespace pkgexec {
namespace {

template <typename Enum>
constexpr std::uint8_t enum_byte(Enum value) noexcept
{
  return static_cast<std::uint8_t>(value);
}

void require_consistent_guarantee(
    execution_guarantee guarantee,
    const resource_layout& resources,
    const environment_policy& environment,
    const resource_limits& limits,
    const cancellation_policy& cancellation)
{
  const bool has_read_only = std::any_of(
      resources.bindings().begin(), resources.bindings().end(),
      [](const resource_binding& value) {
        return value.access() == resource_access::read_only;
      });
  const bool has_writable = std::any_of(
      resources.bindings().begin(), resources.bindings().end(),
      [](const resource_binding& value) {
        return value.access() == resource_access::writable;
      });

  bool consistent = true;
  switch (guarantee) {
    case execution_guarantee::read_only_resources:
      consistent = has_read_only;
      break;
    case execution_guarantee::writable_resources:
      consistent = has_writable;
      break;
    case execution_guarantee::network_denied:
      consistent = environment.network() == network_policy::denied;
      break;
    case execution_guarantee::loopback_isolated:
      consistent = environment.network() == network_policy::loopback_only;
      break;
    case execution_guarantee::resource_limits:
      consistent = !limits.empty();
      break;
    case execution_guarantee::cpu_time_limit:
      consistent = limits.cpu_time_milliseconds().has_value();
      break;
    case execution_guarantee::address_space_limit:
      consistent = limits.address_space_bytes().has_value();
      break;
    case execution_guarantee::file_size_limit:
      consistent = limits.file_size_bytes().has_value();
      break;
    case execution_guarantee::open_files_limit:
      consistent = limits.open_files().has_value();
      break;
    case execution_guarantee::process_count_limit:
      consistent = limits.process_count().has_value();
      break;
    case execution_guarantee::cancellation:
      consistent = cancellation.mode() != cancellation_mode::disabled;
      break;
    case execution_guarantee::complete_stdout_capture:
      consistent = environment.standard_output() == stream_policy::capture_complete;
      break;
    case execution_guarantee::complete_stderr_capture:
      consistent = environment.standard_error() == stream_policy::capture_complete;
      break;
    case execution_guarantee::exact_interpreter:
    case execution_guarantee::closed_environment:
    case execution_guarantee::root_view:
    case execution_guarantee::fixed_credentials:
    case execution_guarantee::cleanup_verified:
      consistent = true;
      break;
  }
  if (!consistent) {
    throw error(error_code::invalid_policy,
                "required execution guarantee conflicts with the request policy: " +
                std::string(to_string(guarantee)));
  }
}

std::vector<execution_guarantee> derive_required_guarantees(
    const resource_layout& resources,
    const environment_policy& environment,
    const resource_limits& limits,
    const cancellation_policy& cancellation,
    std::vector<execution_guarantee> additional)
{
  for (const auto guarantee : additional) {
    if (!detail::valid(guarantee)) {
      throw error(error_code::invalid_policy,
                  "unsupported required execution guarantee");
    }
  }
  additional.push_back(execution_guarantee::exact_interpreter);
  additional.push_back(execution_guarantee::closed_environment);
  additional.push_back(execution_guarantee::root_view);
  additional.push_back(execution_guarantee::fixed_credentials);
  additional.push_back(execution_guarantee::cleanup_verified);

  for (const auto& binding : resources.bindings()) {
    additional.push_back(binding.access() == resource_access::read_only
        ? execution_guarantee::read_only_resources
        : execution_guarantee::writable_resources);
  }
  switch (environment.network()) {
    case network_policy::denied:
      additional.push_back(execution_guarantee::network_denied);
      break;
    case network_policy::loopback_only:
      additional.push_back(execution_guarantee::loopback_isolated);
      break;
    case network_policy::allowed:
      break;
  }
  if (!limits.empty()) {
    additional.push_back(execution_guarantee::resource_limits);
  }
  if (limits.cpu_time_milliseconds()) {
    additional.push_back(execution_guarantee::cpu_time_limit);
  }
  if (limits.address_space_bytes()) {
    additional.push_back(execution_guarantee::address_space_limit);
  }
  if (limits.file_size_bytes()) {
    additional.push_back(execution_guarantee::file_size_limit);
  }
  if (limits.open_files()) {
    additional.push_back(execution_guarantee::open_files_limit);
  }
  if (limits.process_count()) {
    additional.push_back(execution_guarantee::process_count_limit);
  }
  if (cancellation.mode() != cancellation_mode::disabled) {
    additional.push_back(execution_guarantee::cancellation);
  }
  if (environment.standard_output() == stream_policy::capture_complete) {
    additional.push_back(execution_guarantee::complete_stdout_capture);
  }
  if (environment.standard_error() == stream_policy::capture_complete) {
    additional.push_back(execution_guarantee::complete_stderr_capture);
  }

  std::sort(additional.begin(), additional.end());
  additional.erase(std::unique(additional.begin(), additional.end()),
                   additional.end());
  for (const auto guarantee : additional) {
    require_consistent_guarantee(guarantee, resources, environment, limits,
                                 cancellation);
  }
  return additional;
}

execution_request_identity identify_request(
    const pkgsource::program& program,
    const execution_purpose& purpose,
    const interpreter_identity& interpreter,
    const root_view_identity& root_view,
    const resource_layout& resources,
    const environment_policy& environment,
    const credential_policy& credentials,
    const resource_limits& limits,
    const cancellation_policy& cancellation,
    const std::vector<execution_guarantee>& guarantees)
{
  detail::identity_builder hash("pkgexec/execution-request/v1");
  hash.add_u16(execution_request_schema_version);
  hash.add_string(pkgsource::to_string(program.language()));
  hash.add_string(program.content_digest().hex());
  hash.add_u8(enum_byte(purpose.kind()));
  hash.add_bool(purpose.action().has_value());
  if (purpose.action()) {
    hash.add_string(pkgsource::to_string(*purpose.action()));
  }
  hash.add_string(interpreter.hex());
  hash.add_string(root_view.hex());
  hash.add_string(resources.identity().hex());
  hash.add_string(environment.identity().hex());
  hash.add_string(credentials.identity().hex());
  hash.add_string(limits.identity().hex());
  hash.add_u8(enum_byte(cancellation.mode()));
  hash.add_bool(cancellation.grace_period_milliseconds().has_value());
  if (cancellation.grace_period_milliseconds()) {
    hash.add_u64(*cancellation.grace_period_milliseconds());
  }
  hash.add_u64(guarantees.size());
  for (const auto guarantee : guarantees) {
    hash.add_u8(enum_byte(guarantee));
  }
  return execution_request_identity::from_sha256(hash.finish());
}

} // namespace

execution_request::execution_request(
    pkgsource::program program,
    execution_purpose purpose,
    interpreter_identity interpreter,
    root_view_identity root_view,
    resource_layout resources,
    environment_policy environment,
    credential_policy credentials,
    resource_limits limits,
    cancellation_policy cancellation,
    std::vector<execution_guarantee> required_guarantees,
    execution_request_identity identity)
    : program_(std::move(program)), purpose_(std::move(purpose)),
      interpreter_(std::move(interpreter)), root_view_(std::move(root_view)),
      resources_(std::move(resources)), environment_(std::move(environment)),
      credentials_(std::move(credentials)), limits_(std::move(limits)),
      cancellation_(std::move(cancellation)),
      required_guarantees_(std::move(required_guarantees)),
      identity_(std::move(identity))
{
}

execution_request execution_request::seal(
    pkgsource::program program,
    execution_purpose purpose,
    interpreter_identity interpreter,
    root_view_identity root_view,
    resource_layout resources,
    environment_policy environment,
    credential_policy credentials,
    resource_limits limits,
    cancellation_policy cancellation,
    std::vector<execution_guarantee> additional_required_guarantees)
{
  auto guarantees = derive_required_guarantees(
      resources, environment, limits, cancellation,
      std::move(additional_required_guarantees));
  auto identity = identify_request(program, purpose, interpreter, root_view,
                                   resources, environment, credentials, limits,
                                   cancellation, guarantees);
  return execution_request(
      std::move(program), std::move(purpose), std::move(interpreter),
      std::move(root_view), std::move(resources), std::move(environment),
      std::move(credentials), std::move(limits), std::move(cancellation),
      std::move(guarantees), std::move(identity));
}

std::uint16_t execution_request::schema_version() const noexcept
{ return schema_version_; }
const pkgsource::program& execution_request::program() const noexcept
{ return program_; }
const execution_purpose& execution_request::purpose() const noexcept
{ return purpose_; }
const interpreter_identity& execution_request::interpreter() const noexcept
{ return interpreter_; }
const root_view_identity& execution_request::root_view() const noexcept
{ return root_view_; }
const resource_layout& execution_request::resources() const noexcept
{ return resources_; }
const environment_policy& execution_request::environment() const noexcept
{ return environment_; }
const credential_policy& execution_request::credentials() const noexcept
{ return credentials_; }
const resource_limits& execution_request::limits() const noexcept
{ return limits_; }
const cancellation_policy& execution_request::cancellation() const noexcept
{ return cancellation_; }
const std::vector<execution_guarantee>& execution_request::required_guarantees() const noexcept
{ return required_guarantees_; }
const execution_request_identity& execution_request::identity() const noexcept
{ return identity_; }
bool operator==(const execution_request& lhs,
                const execution_request& rhs) noexcept
{ return lhs.identity_ == rhs.identity_; }
bool operator!=(const execution_request& lhs,
                const execution_request& rhs) noexcept
{ return !(lhs == rhs); }

} // namespace pkgexec
