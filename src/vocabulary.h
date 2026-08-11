// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgexec/model.h>

namespace pkgexec::detail {

constexpr bool valid(resource_role value) noexcept
{
  switch (value) {
    case resource_role::source_tree:
    case resource_role::build_input_tree:
    case resource_role::check_input_tree:
    case resource_role::build_workspace:
    case resource_role::package_output_root:
    case resource_role::managed_target_root:
    case resource_role::private_temporary_root:
      return true;
  }
  return false;
}

constexpr bool valid(resource_access value) noexcept
{
  switch (value) {
    case resource_access::read_only:
    case resource_access::writable:
      return true;
  }
  return false;
}

constexpr bool valid(network_policy value) noexcept
{
  switch (value) {
    case network_policy::denied:
    case network_policy::loopback_only:
    case network_policy::allowed:
      return true;
  }
  return false;
}

constexpr bool valid(stdin_policy value) noexcept
{
  switch (value) {
    case stdin_policy::closed:
    case stdin_policy::null_device:
      return true;
  }
  return false;
}

constexpr bool valid(stream_policy value) noexcept
{
  switch (value) {
    case stream_policy::capture_complete:
    case stream_policy::discard:
      return true;
  }
  return false;
}

constexpr bool valid(resource_limit_kind value) noexcept
{
  switch (value) {
    case resource_limit_kind::cpu_time:
    case resource_limit_kind::address_space:
    case resource_limit_kind::file_size:
    case resource_limit_kind::open_files:
    case resource_limit_kind::process_count:
      return true;
  }
  return false;
}

constexpr bool valid(execution_guarantee value) noexcept
{
  switch (value) {
    case execution_guarantee::exact_interpreter:
    case execution_guarantee::closed_environment:
    case execution_guarantee::root_view:
    case execution_guarantee::read_only_resources:
    case execution_guarantee::writable_resources:
    case execution_guarantee::fixed_credentials:
    case execution_guarantee::network_denied:
    case execution_guarantee::loopback_isolated:
    case execution_guarantee::resource_limits:
    case execution_guarantee::cancellation:
    case execution_guarantee::complete_stdout_capture:
    case execution_guarantee::complete_stderr_capture:
    case execution_guarantee::cleanup_verified:
    case execution_guarantee::cpu_time_limit:
    case execution_guarantee::address_space_limit:
    case execution_guarantee::file_size_limit:
    case execution_guarantee::open_files_limit:
    case execution_guarantee::process_count_limit:
      return true;
  }
  return false;
}

constexpr bool valid(cleanup_outcome value) noexcept
{
  switch (value) {
    case cleanup_outcome::not_required:
    case cleanup_outcome::verified:
    case cleanup_outcome::failed:
      return true;
  }
  return false;
}

constexpr bool valid(execution_failure_kind value) noexcept
{
  switch (value) {
    case execution_failure_kind::request_rejected:
    case execution_failure_kind::backend_unsupported:
    case execution_failure_kind::resource_admission_failed:
    case execution_failure_kind::interpreter_unavailable:
    case execution_failure_kind::isolation_setup_failed:
    case execution_failure_kind::process_start_failed:
    case execution_failure_kind::program_exited_nonzero:
    case execution_failure_kind::program_terminated_by_signal:
    case execution_failure_kind::resource_limit_exceeded:
    case execution_failure_kind::cancelled:
    case execution_failure_kind::log_capture_failed:
    case execution_failure_kind::cleanup_failed:
      return true;
  }
  return false;
}

constexpr bool valid(pkgsource::lifecycle_action value) noexcept
{
  switch (value) {
    case pkgsource::lifecycle_action::pre_install:
    case pkgsource::lifecycle_action::post_install:
    case pkgsource::lifecycle_action::pre_remove:
    case pkgsource::lifecycle_action::post_remove:
      return true;
  }
  return false;
}

} // namespace pkgexec::detail
