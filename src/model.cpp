// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/model.h>

#include <libpkgexec/error.h>

#include "identity_support.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

namespace pkgexec {
namespace {

template <typename Enum>
constexpr std::uint8_t enum_byte(Enum value) noexcept
{
  return static_cast<std::uint8_t>(value);
}

bool is_named_role(resource_role role) noexcept
{
  return role == resource_role::source_tree ||
         role == resource_role::build_input_tree ||
         role == resource_role::check_input_tree;
}

bool valid_slot_name(std::string_view value)
{
  if (value.empty()) {
    return false;
  }
  for (std::size_t i = 0; i < value.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(value[i]);
    const bool alnum = std::isalnum(ch) != 0;
    if (i == 0U && !alnum) {
      return false;
    }
    if (!alnum && ch != '.' && ch != '_' && ch != '+' && ch != '-') {
      return false;
    }
  }
  return true;
}

bool valid_environment_name(std::string_view value)
{
  if (value.empty()) {
    return false;
  }
  const unsigned char first = static_cast<unsigned char>(value.front());
  if (!(std::isalpha(first) != 0 || first == '_')) {
    return false;
  }
  for (const char raw : value.substr(1)) {
    const unsigned char ch = static_cast<unsigned char>(raw);
    if (!(std::isalnum(ch) != 0 || ch == '_')) {
      return false;
    }
  }
  return true;
}

bool contains_nul(std::string_view value) noexcept
{
  return value.find('\0') != std::string_view::npos;
}

bool reserved_environment_name(std::string_view value) noexcept
{
  static constexpr std::array<std::string_view, 8> reserved{{
      "PATH", "HOME", "LANG", "LC_ALL", "TZ", "TMPDIR",
      "SOURCE_DATE_EPOCH", "PKGEXEC_JOBS",
  }};
  return std::find(reserved.begin(), reserved.end(), value) != reserved.end();
}

void require_positive(const std::optional<std::uint64_t>& value,
                      const char* field)
{
  if (value && *value == 0U) {
    throw error(error_code::invalid_policy,
                std::string(field) + " must be greater than zero");
  }
}

resource_layout_identity identify_layout(
    const std::vector<resource_binding>& bindings,
    const resource_slot& working_directory)
{
  detail::identity_builder hash("pkgexec/resource-layout/v1");
  hash.add_u64(bindings.size());
  for (const auto& binding : bindings) {
    hash.add_u8(enum_byte(binding.slot().role()));
    hash.add_string(binding.slot().name());
    hash.add_string(binding.resource().hex());
    hash.add_u8(enum_byte(binding.access()));
    hash.add_string(binding.mount_point().string());
  }
  hash.add_u8(enum_byte(working_directory.role()));
  hash.add_string(working_directory.name());
  return resource_layout_identity::from_sha256(hash.finish());
}

environment_policy_identity identify_environment(
    const std::vector<logical_path>& executable_search_path,
    const logical_path& home_directory,
    const logical_path& temporary_directory,
    std::uint32_t parallelism,
    std::uint32_t file_creation_mask,
    const std::optional<std::int64_t>& source_date_epoch,
    network_policy network,
    stdin_policy standard_input,
    stream_policy standard_output,
    stream_policy standard_error,
    const std::vector<environment_variable>& additional_variables)
{
  detail::identity_builder hash("pkgexec/environment-policy/v1");
  hash.add_u8(enum_byte(locale_policy::c_utf8));
  hash.add_u8(enum_byte(timezone_policy::utc));
  hash.add_u8(enum_byte(home_policy::isolated));
  hash.add_u8(enum_byte(network));
  hash.add_u8(enum_byte(standard_input));
  hash.add_u8(enum_byte(standard_output));
  hash.add_u8(enum_byte(standard_error));
  hash.add_u64(executable_search_path.size());
  for (const auto& path : executable_search_path) {
    hash.add_string(path.string());
  }
  hash.add_string(home_directory.string());
  hash.add_string(temporary_directory.string());
  hash.add_u32(parallelism);
  hash.add_u32(file_creation_mask);
  hash.add_optional_i64(source_date_epoch);
  hash.add_u64(additional_variables.size());
  for (const auto& variable : additional_variables) {
    hash.add_string(variable.name());
    hash.add_string(variable.value());
  }
  return environment_policy_identity::from_sha256(hash.finish());
}

credential_policy_identity identify_credentials(
    std::uint64_t user_id,
    std::uint64_t group_id,
    const std::vector<std::uint64_t>& supplementary_groups,
    bool no_new_privileges)
{
  detail::identity_builder hash("pkgexec/credential-policy/v1");
  hash.add_u64(user_id);
  hash.add_u64(group_id);
  hash.add_u64(supplementary_groups.size());
  for (const auto group : supplementary_groups) {
    hash.add_u64(group);
  }
  hash.add_bool(no_new_privileges);
  return credential_policy_identity::from_sha256(hash.finish());
}

resource_limits_identity identify_limits(
    const std::optional<std::uint64_t>& cpu_time_milliseconds,
    const std::optional<std::uint64_t>& address_space_bytes,
    const std::optional<std::uint64_t>& file_size_bytes,
    const std::optional<std::uint64_t>& open_files,
    const std::optional<std::uint64_t>& process_count)
{
  detail::identity_builder hash("pkgexec/resource-limits/v1");
  hash.add_optional_u64(cpu_time_milliseconds);
  hash.add_optional_u64(address_space_bytes);
  hash.add_optional_u64(file_size_bytes);
  hash.add_optional_u64(open_files);
  hash.add_optional_u64(process_count);
  return resource_limits_identity::from_sha256(hash.finish());
}

} // namespace

#define PKGEXEC_ENUM_STRING_CASE(value) case value: return #value

std::string_view to_string(execution_purpose_kind value) noexcept
{
  switch (value) {
    case execution_purpose_kind::build: return "build";
    case execution_purpose_kind::check: return "check";
    case execution_purpose_kind::lifecycle: return "lifecycle";
  }
  return "unknown";
}

std::string_view to_string(resource_role value) noexcept
{
  switch (value) {
    case resource_role::source_tree: return "source-tree";
    case resource_role::build_input_tree: return "build-input-tree";
    case resource_role::check_input_tree: return "check-input-tree";
    case resource_role::build_workspace: return "build-workspace";
    case resource_role::package_output_root: return "package-output-root";
    case resource_role::managed_target_root: return "managed-target-root";
    case resource_role::private_temporary_root: return "private-temporary-root";
  }
  return "unknown";
}

std::string_view to_string(resource_access value) noexcept
{
  return value == resource_access::read_only ? "read-only" : "writable";
}
std::string_view to_string(locale_policy) noexcept { return "C.UTF-8"; }
std::string_view to_string(timezone_policy) noexcept { return "UTC"; }
std::string_view to_string(home_policy) noexcept { return "isolated"; }
std::string_view to_string(network_policy value) noexcept
{
  switch (value) {
    case network_policy::denied: return "denied";
    case network_policy::loopback_only: return "loopback-only";
    case network_policy::allowed: return "allowed";
  }
  return "unknown";
}
std::string_view to_string(stdin_policy value) noexcept
{
  return value == stdin_policy::closed ? "closed" : "null-device";
}
std::string_view to_string(stream_policy value) noexcept
{
  return value == stream_policy::capture_complete ? "capture-complete" : "discard";
}
std::string_view to_string(cancellation_mode value) noexcept
{
  return value == cancellation_mode::disabled ? "disabled" : "graceful-then-forced";
}
std::string_view to_string(resource_limit_kind value) noexcept
{
  switch (value) {
    case resource_limit_kind::cpu_time: return "cpu-time";
    case resource_limit_kind::address_space: return "address-space";
    case resource_limit_kind::file_size: return "file-size";
    case resource_limit_kind::open_files: return "open-files";
    case resource_limit_kind::process_count: return "process-count";
  }
  return "unknown";
}
std::string_view to_string(execution_guarantee value) noexcept
{
  switch (value) {
    case execution_guarantee::exact_interpreter: return "exact-interpreter";
    case execution_guarantee::closed_environment: return "closed-environment";
    case execution_guarantee::root_view: return "root-view";
    case execution_guarantee::read_only_resources: return "read-only-resources";
    case execution_guarantee::writable_resources: return "writable-resources";
    case execution_guarantee::fixed_credentials: return "fixed-credentials";
    case execution_guarantee::network_denied: return "network-denied";
    case execution_guarantee::loopback_isolated: return "loopback-isolated";
    case execution_guarantee::resource_limits: return "resource-limits";
    case execution_guarantee::cancellation: return "cancellation";
    case execution_guarantee::complete_stdout_capture: return "complete-stdout-capture";
    case execution_guarantee::complete_stderr_capture: return "complete-stderr-capture";
    case execution_guarantee::cleanup_verified: return "cleanup-verified";
  }
  return "unknown";
}
std::string_view to_string(process_termination_kind value) noexcept
{
  switch (value) {
    case process_termination_kind::exited: return "exited";
    case process_termination_kind::signaled: return "signaled";
    case process_termination_kind::cancelled: return "cancelled";
    case process_termination_kind::resource_limited: return "resource-limited";
  }
  return "unknown";
}
std::string_view to_string(execution_status value) noexcept
{
  return value == execution_status::succeeded ? "succeeded" : "failed";
}
std::string_view to_string(execution_start_state value) noexcept
{
  return value == execution_start_state::started ? "started" : "not-started";
}
std::string_view to_string(cleanup_outcome value) noexcept
{
  switch (value) {
    case cleanup_outcome::not_required: return "not-required";
    case cleanup_outcome::verified: return "verified";
    case cleanup_outcome::failed: return "failed";
  }
  return "unknown";
}
std::string_view to_string(execution_failure_kind value) noexcept
{
  switch (value) {
    case execution_failure_kind::request_rejected: return "request-rejected";
    case execution_failure_kind::backend_unsupported: return "backend-unsupported";
    case execution_failure_kind::resource_admission_failed: return "resource-admission-failed";
    case execution_failure_kind::interpreter_unavailable: return "interpreter-unavailable";
    case execution_failure_kind::isolation_setup_failed: return "isolation-setup-failed";
    case execution_failure_kind::process_start_failed: return "process-start-failed";
    case execution_failure_kind::program_exited_nonzero: return "program-exited-nonzero";
    case execution_failure_kind::program_terminated_by_signal: return "program-terminated-by-signal";
    case execution_failure_kind::resource_limit_exceeded: return "resource-limit-exceeded";
    case execution_failure_kind::cancelled: return "cancelled";
    case execution_failure_kind::log_capture_failed: return "log-capture-failed";
    case execution_failure_kind::cleanup_failed: return "cleanup-failed";
  }
  return "unknown";
}

sha256_digest::sha256_digest(std::string hex) : hex_(std::move(hex))
{
  detail::require_sha256_hex(hex_);
}
sha256_digest sha256_digest::of_bytes(std::string_view bytes)
{
  return sha256_digest(detail::sha256_hex(bytes));
}
const std::string& sha256_digest::hex() const noexcept { return hex_; }
bool operator==(const sha256_digest& lhs, const sha256_digest& rhs) noexcept
{ return lhs.hex_ == rhs.hex_; }
bool operator!=(const sha256_digest& lhs, const sha256_digest& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const sha256_digest& lhs, const sha256_digest& rhs) noexcept
{ return lhs.hex_ < rhs.hex_; }

execution_purpose::execution_purpose(
    execution_purpose_kind kind,
    std::optional<pkgsource::lifecycle_action> action)
    : kind_(kind), action_(action)
{
  if ((kind_ == execution_purpose_kind::lifecycle) != action_.has_value()) {
    throw error(error_code::invalid_purpose,
                "only lifecycle execution purposes carry a lifecycle action");
  }
}
execution_purpose execution_purpose::build()
{ return execution_purpose(execution_purpose_kind::build, std::nullopt); }
execution_purpose execution_purpose::check()
{ return execution_purpose(execution_purpose_kind::check, std::nullopt); }
execution_purpose execution_purpose::lifecycle(pkgsource::lifecycle_action action)
{ return execution_purpose(execution_purpose_kind::lifecycle, action); }
execution_purpose_kind execution_purpose::kind() const noexcept { return kind_; }
const std::optional<pkgsource::lifecycle_action>& execution_purpose::action() const noexcept
{ return action_; }
bool operator==(const execution_purpose& lhs,
                const execution_purpose& rhs) noexcept
{ return lhs.kind_ == rhs.kind_ && lhs.action_ == rhs.action_; }
bool operator!=(const execution_purpose& lhs,
                const execution_purpose& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const execution_purpose& lhs,
               const execution_purpose& rhs) noexcept
{ return std::tie(lhs.kind_, lhs.action_) < std::tie(rhs.kind_, rhs.action_); }

logical_path::logical_path(std::string value) : value_(std::move(value)) {}
logical_path logical_path::parse(std::string_view value)
{
  if (value.empty() || value.front() != '/' || contains_nul(value)) {
    throw error(error_code::invalid_path,
                "logical paths must be non-empty absolute POSIX paths");
  }
  if (value == "/") {
    return logical_path(std::string(value));
  }
  if (value.size() > 1U && value.back() == '/') {
    throw error(error_code::invalid_path,
                "logical paths must not contain a trailing slash");
  }
  std::size_t begin = 1U;
  while (begin <= value.size()) {
    const auto end = value.find('/', begin);
    const auto length = (end == std::string_view::npos)
        ? value.size() - begin : end - begin;
    const auto component = value.substr(begin, length);
    if (component.empty() || component == "." || component == "..") {
      throw error(error_code::invalid_path,
                  "logical paths must be normalized without empty, dot, or parent components");
    }
    if (end == std::string_view::npos) {
      break;
    }
    begin = end + 1U;
  }
  return logical_path(std::string(value));
}
const std::string& logical_path::string() const noexcept { return value_; }
bool operator==(const logical_path& lhs, const logical_path& rhs) noexcept
{ return lhs.value_ == rhs.value_; }
bool operator!=(const logical_path& lhs, const logical_path& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const logical_path& lhs, const logical_path& rhs) noexcept
{ return lhs.value_ < rhs.value_; }

resource_slot::resource_slot(resource_role role, std::string name)
    : role_(role), name_(std::move(name))
{
}
resource_slot resource_slot::singleton(resource_role role)
{
  if (is_named_role(role)) {
    throw error(error_code::invalid_value,
                "source and package-input resource roles require a canonical name");
  }
  return resource_slot(role, {});
}
resource_slot resource_slot::named(resource_role role, std::string name)
{
  if (!is_named_role(role)) {
    throw error(error_code::invalid_value,
                "only source and package-input resource roles may be named");
  }
  if (!valid_slot_name(name)) {
    throw error(error_code::invalid_value,
                "resource slot names must be canonical ASCII tokens");
  }
  return resource_slot(role, std::move(name));
}
resource_role resource_slot::role() const noexcept { return role_; }
const std::string& resource_slot::name() const noexcept { return name_; }
std::string resource_slot::text() const
{
  std::string value(to_string(role_));
  if (!name_.empty()) {
    value += ':';
    value += name_;
  }
  return value;
}
bool operator==(const resource_slot& lhs, const resource_slot& rhs) noexcept
{ return lhs.role_ == rhs.role_ && lhs.name_ == rhs.name_; }
bool operator!=(const resource_slot& lhs, const resource_slot& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const resource_slot& lhs, const resource_slot& rhs) noexcept
{ return std::tie(lhs.role_, lhs.name_) < std::tie(rhs.role_, rhs.name_); }

resource_binding::resource_binding(resource_slot slot,
                                   resource_identity resource,
                                   resource_access access,
                                   logical_path mount_point)
    : slot_(std::move(slot)), resource_(std::move(resource)), access_(access),
      mount_point_(std::move(mount_point))
{
  const auto role = slot_.role();
  if ((role == resource_role::source_tree ||
       role == resource_role::build_input_tree ||
       role == resource_role::check_input_tree) &&
      access_ != resource_access::read_only) {
    throw error(error_code::invalid_policy,
                "source and package-input resources must be read-only");
  }
  if ((role == resource_role::build_workspace ||
       role == resource_role::package_output_root ||
       role == resource_role::private_temporary_root) &&
      access_ != resource_access::writable) {
    throw error(error_code::invalid_policy,
                "workspace, package-output, and temporary resources must be writable");
  }
}
const resource_slot& resource_binding::slot() const noexcept { return slot_; }
const resource_identity& resource_binding::resource() const noexcept { return resource_; }
resource_access resource_binding::access() const noexcept { return access_; }
const logical_path& resource_binding::mount_point() const noexcept { return mount_point_; }
bool operator==(const resource_binding& lhs, const resource_binding& rhs) noexcept
{
  return lhs.slot_ == rhs.slot_ && lhs.resource_ == rhs.resource_ &&
         lhs.access_ == rhs.access_ && lhs.mount_point_ == rhs.mount_point_;
}
bool operator!=(const resource_binding& lhs, const resource_binding& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const resource_binding& lhs, const resource_binding& rhs) noexcept
{
  return std::tie(lhs.slot_, lhs.resource_, lhs.access_, lhs.mount_point_) <
         std::tie(rhs.slot_, rhs.resource_, rhs.access_, rhs.mount_point_);
}

resource_layout::resource_layout(std::vector<resource_binding> bindings,
                                 resource_slot working_directory,
                                 resource_layout_identity identity)
    : bindings_(std::move(bindings)),
      working_directory_(std::move(working_directory)),
      identity_(std::move(identity))
{
}
resource_layout resource_layout::seal(
    std::vector<resource_binding> bindings,
    resource_slot working_directory)
{
  if (bindings.empty()) {
    throw error(error_code::missing_working_directory,
                "execution resource layouts cannot be empty");
  }
  std::sort(bindings.begin(), bindings.end());
  for (std::size_t i = 1; i < bindings.size(); ++i) {
    if (bindings[i - 1].slot() == bindings[i].slot()) {
      throw error(error_code::duplicate_resource,
                  "duplicate execution resource slot " + bindings[i].slot().text());
    }
  }
  std::set<std::string> mount_points;
  bool working_found = false;
  for (const auto& binding : bindings) {
    if (!mount_points.insert(binding.mount_point().string()).second) {
      throw error(error_code::duplicate_mount_point,
                  "multiple execution resources use mount point " +
                  binding.mount_point().string());
    }
    working_found = working_found || binding.slot() == working_directory;
  }
  if (!working_found) {
    throw error(error_code::missing_working_directory,
                "working-directory resource is absent from the resource layout");
  }
  auto identity = identify_layout(bindings, working_directory);
  return resource_layout(std::move(bindings), std::move(working_directory),
                         std::move(identity));
}
const std::vector<resource_binding>& resource_layout::bindings() const noexcept
{ return bindings_; }
const resource_slot& resource_layout::working_directory() const noexcept
{ return working_directory_; }
const resource_layout_identity& resource_layout::identity() const noexcept
{ return identity_; }
const resource_binding& resource_layout::binding(const resource_slot& slot) const
{
  const auto found = std::lower_bound(
      bindings_.begin(), bindings_.end(), slot,
      [](const resource_binding& binding, const resource_slot& key) {
        return binding.slot() < key;
      });
  if (found == bindings_.end() || found->slot() != slot) {
    throw error(error_code::invalid_value,
                "unknown execution resource slot " + slot.text());
  }
  return *found;
}
bool operator==(const resource_layout& lhs, const resource_layout& rhs) noexcept
{ return lhs.bindings_ == rhs.bindings_ && lhs.working_directory_ == rhs.working_directory_; }
bool operator!=(const resource_layout& lhs, const resource_layout& rhs) noexcept
{ return !(lhs == rhs); }

environment_variable::environment_variable(std::string name, std::string value)
    : name_(std::move(name)), value_(std::move(value))
{
  if (!valid_environment_name(name_) || contains_nul(value_)) {
    throw error(error_code::invalid_policy,
                "environment variables require a valid name and NUL-free value");
  }
  if (reserved_environment_name(name_)) {
    throw error(error_code::reserved_environment_variable,
                "environment variable " + name_ + " is owned by the closed policy");
  }
}
const std::string& environment_variable::name() const noexcept { return name_; }
const std::string& environment_variable::value() const noexcept { return value_; }
bool operator==(const environment_variable& lhs,
                const environment_variable& rhs) noexcept
{ return lhs.name_ == rhs.name_ && lhs.value_ == rhs.value_; }
bool operator!=(const environment_variable& lhs,
                const environment_variable& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const environment_variable& lhs,
               const environment_variable& rhs) noexcept
{ return std::tie(lhs.name_, lhs.value_) < std::tie(rhs.name_, rhs.value_); }

environment_policy::environment_policy(
    std::vector<logical_path> executable_search_path,
    logical_path home_directory,
    logical_path temporary_directory,
    std::uint32_t parallelism,
    std::uint32_t file_creation_mask,
    std::optional<std::int64_t> source_date_epoch,
    network_policy network,
    stdin_policy standard_input,
    stream_policy standard_output,
    stream_policy standard_error,
    std::vector<environment_variable> additional_variables,
    environment_policy_identity identity)
    : executable_search_path_(std::move(executable_search_path)),
      home_directory_(std::move(home_directory)),
      temporary_directory_(std::move(temporary_directory)),
      parallelism_(parallelism), file_creation_mask_(file_creation_mask),
      source_date_epoch_(source_date_epoch), network_(network),
      standard_input_(standard_input), standard_output_(standard_output),
      standard_error_(standard_error),
      additional_variables_(std::move(additional_variables)),
      identity_(std::move(identity))
{
}
environment_policy environment_policy::hermetic(
    std::vector<logical_path> executable_search_path,
    logical_path home_directory,
    logical_path temporary_directory,
    std::uint32_t parallelism,
    std::uint32_t file_creation_mask,
    std::optional<std::int64_t> source_date_epoch,
    network_policy network,
    stdin_policy standard_input,
    stream_policy standard_output,
    stream_policy standard_error,
    std::vector<environment_variable> additional_variables)
{
  if (executable_search_path.empty()) {
    throw error(error_code::invalid_policy,
                "closed execution PATH cannot be empty");
  }
  if (parallelism == 0U) {
    throw error(error_code::invalid_policy,
                "execution parallelism must be greater than zero");
  }
  if (file_creation_mask > 0777U) {
    throw error(error_code::invalid_policy,
                "file creation mask must fit POSIX permission bits");
  }
  if (source_date_epoch && *source_date_epoch < 0) {
    throw error(error_code::invalid_policy,
                "SOURCE_DATE_EPOCH cannot be negative");
  }
  std::set<std::string> paths;
  for (const auto& path : executable_search_path) {
    if (!paths.insert(path.string()).second) {
      throw error(error_code::invalid_policy,
                  "closed execution PATH contains a duplicate entry");
    }
  }
  std::sort(additional_variables.begin(), additional_variables.end());
  for (std::size_t i = 1; i < additional_variables.size(); ++i) {
    if (additional_variables[i - 1].name() == additional_variables[i].name()) {
      throw error(error_code::duplicate_environment_variable,
                  "duplicate admitted environment variable " +
                  additional_variables[i].name());
    }
  }
  auto identity = identify_environment(
      executable_search_path, home_directory, temporary_directory,
      parallelism, file_creation_mask, source_date_epoch, network,
      standard_input, standard_output, standard_error, additional_variables);
  return environment_policy(
      std::move(executable_search_path), std::move(home_directory),
      std::move(temporary_directory), parallelism, file_creation_mask,
      source_date_epoch, network, standard_input, standard_output,
      standard_error, std::move(additional_variables), std::move(identity));
}
locale_policy environment_policy::locale() const noexcept { return locale_policy::c_utf8; }
timezone_policy environment_policy::timezone() const noexcept { return timezone_policy::utc; }
home_policy environment_policy::home() const noexcept { return home_policy::isolated; }
network_policy environment_policy::network() const noexcept { return network_; }
stdin_policy environment_policy::standard_input() const noexcept { return standard_input_; }
stream_policy environment_policy::standard_output() const noexcept { return standard_output_; }
stream_policy environment_policy::standard_error() const noexcept { return standard_error_; }
const std::vector<logical_path>& environment_policy::executable_search_path() const noexcept
{ return executable_search_path_; }
const logical_path& environment_policy::home_directory() const noexcept { return home_directory_; }
const logical_path& environment_policy::temporary_directory() const noexcept { return temporary_directory_; }
std::uint32_t environment_policy::parallelism() const noexcept { return parallelism_; }
std::uint32_t environment_policy::file_creation_mask() const noexcept { return file_creation_mask_; }
const std::optional<std::int64_t>& environment_policy::source_date_epoch() const noexcept
{ return source_date_epoch_; }
const std::vector<environment_variable>& environment_policy::additional_variables() const noexcept
{ return additional_variables_; }
const environment_policy_identity& environment_policy::identity() const noexcept
{ return identity_; }
bool operator==(const environment_policy& lhs,
                const environment_policy& rhs) noexcept
{ return lhs.identity_ == rhs.identity_; }
bool operator!=(const environment_policy& lhs,
                const environment_policy& rhs) noexcept
{ return !(lhs == rhs); }

credential_policy::credential_policy(
    std::uint64_t user_id,
    std::uint64_t group_id,
    std::vector<std::uint64_t> supplementary_groups,
    bool no_new_privileges,
    credential_policy_identity identity)
    : user_id_(user_id), group_id_(group_id),
      supplementary_groups_(std::move(supplementary_groups)),
      no_new_privileges_(no_new_privileges), identity_(std::move(identity))
{
}
credential_policy credential_policy::fixed(
    std::uint64_t user_id,
    std::uint64_t group_id,
    std::vector<std::uint64_t> supplementary_groups,
    bool no_new_privileges)
{
  std::sort(supplementary_groups.begin(), supplementary_groups.end());
  if (std::adjacent_find(supplementary_groups.begin(),
                         supplementary_groups.end()) != supplementary_groups.end()) {
    throw error(error_code::invalid_policy,
                "supplementary credential groups must be unique");
  }
  if (std::binary_search(supplementary_groups.begin(),
                         supplementary_groups.end(), group_id)) {
    throw error(error_code::invalid_policy,
                "primary group must not be repeated as a supplementary group");
  }
  auto identity = identify_credentials(user_id, group_id,
                                       supplementary_groups,
                                       no_new_privileges);
  return credential_policy(user_id, group_id, std::move(supplementary_groups),
                           no_new_privileges, std::move(identity));
}
std::uint64_t credential_policy::user_id() const noexcept { return user_id_; }
std::uint64_t credential_policy::group_id() const noexcept { return group_id_; }
const std::vector<std::uint64_t>& credential_policy::supplementary_groups() const noexcept
{ return supplementary_groups_; }
bool credential_policy::no_new_privileges() const noexcept { return no_new_privileges_; }
const credential_policy_identity& credential_policy::identity() const noexcept
{ return identity_; }
bool operator==(const credential_policy& lhs,
                const credential_policy& rhs) noexcept
{ return lhs.identity_ == rhs.identity_; }
bool operator!=(const credential_policy& lhs,
                const credential_policy& rhs) noexcept
{ return !(lhs == rhs); }

resource_limits::resource_limits(
    std::optional<std::uint64_t> cpu_time_milliseconds,
    std::optional<std::uint64_t> address_space_bytes,
    std::optional<std::uint64_t> file_size_bytes,
    std::optional<std::uint64_t> open_files,
    std::optional<std::uint64_t> process_count,
    resource_limits_identity identity)
    : cpu_time_milliseconds_(cpu_time_milliseconds),
      address_space_bytes_(address_space_bytes),
      file_size_bytes_(file_size_bytes), open_files_(open_files),
      process_count_(process_count), identity_(std::move(identity))
{
}
resource_limits resource_limits::make(
    std::optional<std::uint64_t> cpu_time_milliseconds,
    std::optional<std::uint64_t> address_space_bytes,
    std::optional<std::uint64_t> file_size_bytes,
    std::optional<std::uint64_t> open_files,
    std::optional<std::uint64_t> process_count)
{
  require_positive(cpu_time_milliseconds, "CPU time limit");
  require_positive(address_space_bytes, "address-space limit");
  require_positive(file_size_bytes, "file-size limit");
  require_positive(open_files, "open-file limit");
  require_positive(process_count, "process-count limit");
  auto identity = identify_limits(cpu_time_milliseconds, address_space_bytes,
                                  file_size_bytes, open_files, process_count);
  return resource_limits(cpu_time_milliseconds, address_space_bytes,
                         file_size_bytes, open_files, process_count,
                         std::move(identity));
}
const std::optional<std::uint64_t>& resource_limits::cpu_time_milliseconds() const noexcept
{ return cpu_time_milliseconds_; }
const std::optional<std::uint64_t>& resource_limits::address_space_bytes() const noexcept
{ return address_space_bytes_; }
const std::optional<std::uint64_t>& resource_limits::file_size_bytes() const noexcept
{ return file_size_bytes_; }
const std::optional<std::uint64_t>& resource_limits::open_files() const noexcept
{ return open_files_; }
const std::optional<std::uint64_t>& resource_limits::process_count() const noexcept
{ return process_count_; }
bool resource_limits::empty() const noexcept
{
  return !cpu_time_milliseconds_ && !address_space_bytes_ && !file_size_bytes_ &&
         !open_files_ && !process_count_;
}
const resource_limits_identity& resource_limits::identity() const noexcept
{ return identity_; }
bool operator==(const resource_limits& lhs, const resource_limits& rhs) noexcept
{ return lhs.identity_ == rhs.identity_; }
bool operator!=(const resource_limits& lhs, const resource_limits& rhs) noexcept
{ return !(lhs == rhs); }

cancellation_policy::cancellation_policy(
    cancellation_mode mode,
    std::optional<std::uint64_t> grace_period_milliseconds)
    : mode_(mode), grace_period_milliseconds_(grace_period_milliseconds)
{
  if ((mode_ == cancellation_mode::disabled) ==
      grace_period_milliseconds_.has_value()) {
    throw error(error_code::invalid_policy,
                "only enabled cancellation carries a grace period");
  }
  if (grace_period_milliseconds_ && *grace_period_milliseconds_ == 0U) {
    throw error(error_code::invalid_policy,
                "cancellation grace period must be greater than zero");
  }
}
cancellation_policy cancellation_policy::disabled()
{ return cancellation_policy(cancellation_mode::disabled, std::nullopt); }
cancellation_policy cancellation_policy::graceful_then_forced(
    std::uint64_t grace_period_milliseconds)
{
  return cancellation_policy(cancellation_mode::graceful_then_forced,
                             grace_period_milliseconds);
}
cancellation_mode cancellation_policy::mode() const noexcept { return mode_; }
const std::optional<std::uint64_t>& cancellation_policy::grace_period_milliseconds() const noexcept
{ return grace_period_milliseconds_; }
bool operator==(const cancellation_policy& lhs,
                const cancellation_policy& rhs) noexcept
{ return lhs.mode_ == rhs.mode_ && lhs.grace_period_milliseconds_ == rhs.grace_period_milliseconds_; }
bool operator!=(const cancellation_policy& lhs,
                const cancellation_policy& rhs) noexcept
{ return !(lhs == rhs); }

process_termination::process_termination(
    process_termination_kind kind,
    std::optional<std::uint32_t> value,
    std::optional<resource_limit_kind> limit)
    : kind_(kind), value_(value), limit_(limit)
{
}
process_termination process_termination::exited(std::uint32_t status)
{
  if (status > 255U) {
    throw error(error_code::invalid_value,
                "process exit status must fit one byte");
  }
  return process_termination(process_termination_kind::exited, status,
                             std::nullopt);
}
process_termination process_termination::signaled(std::uint32_t signal)
{
  if (signal == 0U) {
    throw error(error_code::invalid_value,
                "terminating signal number must be greater than zero");
  }
  return process_termination(process_termination_kind::signaled, signal,
                             std::nullopt);
}
process_termination process_termination::cancelled()
{
  return process_termination(process_termination_kind::cancelled,
                             std::nullopt, std::nullopt);
}
process_termination process_termination::resource_limited(resource_limit_kind limit)
{
  return process_termination(process_termination_kind::resource_limited,
                             std::nullopt, limit);
}
process_termination_kind process_termination::kind() const noexcept { return kind_; }
const std::optional<std::uint32_t>& process_termination::value() const noexcept
{ return value_; }
const std::optional<resource_limit_kind>& process_termination::limit() const noexcept
{ return limit_; }
bool operator==(const process_termination& lhs,
                const process_termination& rhs) noexcept
{ return lhs.kind_ == rhs.kind_ && lhs.value_ == rhs.value_ && lhs.limit_ == rhs.limit_; }
bool operator!=(const process_termination& lhs,
                const process_termination& rhs) noexcept
{ return !(lhs == rhs); }

stream_capture::stream_capture(std::uint64_t byte_count,
                               sha256_digest digest,
                               std::optional<std::string> material)
    : byte_count_(byte_count), digest_(std::move(digest)),
      material_(std::move(material))
{
  if (material_) {
    if (material_->size() != byte_count_) {
      throw error(error_code::invalid_value,
                  "retained stream material length does not match byte count");
    }
    if (sha256_digest::of_bytes(*material_) != digest_) {
      throw error(error_code::invalid_value,
                  "retained stream material does not match its digest");
    }
  }
}
stream_capture stream_capture::retained(std::string material)
{
  const auto size = static_cast<std::uint64_t>(material.size());
  auto digest = sha256_digest::of_bytes(material);
  return stream_capture(size, std::move(digest), std::move(material));
}
stream_capture stream_capture::observed(std::uint64_t byte_count,
                                        sha256_digest digest)
{
  return stream_capture(byte_count, std::move(digest), std::nullopt);
}
std::uint64_t stream_capture::byte_count() const noexcept { return byte_count_; }
const sha256_digest& stream_capture::digest() const noexcept { return digest_; }
const std::optional<std::string>& stream_capture::material() const noexcept
{ return material_; }
bool operator==(const stream_capture& lhs, const stream_capture& rhs) noexcept
{
  return lhs.byte_count_ == rhs.byte_count_ && lhs.digest_ == rhs.digest_ &&
         lhs.material_ == rhs.material_;
}
bool operator!=(const stream_capture& lhs, const stream_capture& rhs) noexcept
{ return !(lhs == rhs); }

} // namespace pkgexec
