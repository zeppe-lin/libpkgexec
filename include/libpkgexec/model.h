// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file model.h
 *  \brief Executor-neutral native execution values.
 */
#pragma once

#include <libpkgexec/export.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <libpkgexec/identity.h>
#include <libpkgsource/model.h>

namespace pkgexec {

enum class execution_purpose_kind { build, check, lifecycle };
enum class resource_role {
  source_tree,
  build_input_tree,
  check_input_tree,
  build_workspace,
  package_output_root,
  managed_target_root,
  private_temporary_root,
};
enum class resource_access { read_only, writable };
enum class locale_policy { c_utf8 };
enum class timezone_policy { utc };
enum class home_policy { isolated };
enum class network_policy { denied, loopback_only, allowed };
enum class stdin_policy { closed, null_device };
enum class stream_policy { capture_complete, discard };
enum class cancellation_mode { disabled, graceful_then_forced };
enum class resource_limit_kind {
  cpu_time,
  address_space,
  file_size,
  open_files,
  process_count,
};
enum class execution_guarantee {
  exact_interpreter,
  closed_environment,
  root_view,
  read_only_resources,
  writable_resources,
  fixed_credentials,
  network_denied,
  loopback_isolated,
  resource_limits,
  cancellation,
  complete_stdout_capture,
  complete_stderr_capture,
  cleanup_verified,
  cpu_time_limit,
  address_space_limit,
  file_size_limit,
  open_files_limit,
  process_count_limit,
};

enum class process_termination_kind {
  exited,
  signaled,
  cancelled,
  resource_limited,
};

enum class execution_status { succeeded, failed };
enum class execution_start_state { not_started, started };
enum class cleanup_outcome { not_required, verified, failed };
enum class execution_failure_kind {
  request_rejected,
  backend_unsupported,
  resource_admission_failed,
  interpreter_unavailable,
  isolation_setup_failed,
  process_start_failed,
  program_exited_nonzero,
  program_terminated_by_signal,
  resource_limit_exceeded,
  cancelled,
  log_capture_failed,
  cleanup_failed,
};

[[nodiscard]] PKGEXEC_API std::string_view to_string(execution_purpose_kind value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(resource_role value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(resource_access value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(locale_policy value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(timezone_policy value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(home_policy value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(network_policy value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(stdin_policy value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(stream_policy value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(cancellation_mode value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(resource_limit_kind value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(execution_guarantee value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(process_termination_kind value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(execution_status value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(execution_start_state value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(cleanup_outcome value) noexcept;
[[nodiscard]] PKGEXEC_API std::string_view to_string(execution_failure_kind value) noexcept;

/*! \brief Canonical exact-byte SHA-256 value. */
class PKGEXEC_API sha256_digest final {
public:
  explicit sha256_digest(std::string hex);
  [[nodiscard]] static sha256_digest of_bytes(std::string_view bytes);
  [[nodiscard]] const std::string& hex() const noexcept;
  friend PKGEXEC_API bool operator==(const sha256_digest& lhs,
                         const sha256_digest& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const sha256_digest& lhs,
                         const sha256_digest& rhs) noexcept;
  friend PKGEXEC_API bool operator<(const sha256_digest& lhs,
                        const sha256_digest& rhs) noexcept;
private:
  std::string hex_;
};

/*! \brief Exact typed execution purpose. */
class PKGEXEC_API execution_purpose final {
public:
  [[nodiscard]] static execution_purpose build();
  [[nodiscard]] static execution_purpose check();
  [[nodiscard]] static execution_purpose lifecycle(pkgsource::lifecycle_action action);
  [[nodiscard]] execution_purpose_kind kind() const noexcept;
  [[nodiscard]] const std::optional<pkgsource::lifecycle_action>& action() const noexcept;
  friend PKGEXEC_API bool operator==(const execution_purpose& lhs,
                         const execution_purpose& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const execution_purpose& lhs,
                         const execution_purpose& rhs) noexcept;
  friend PKGEXEC_API bool operator<(const execution_purpose& lhs,
                        const execution_purpose& rhs) noexcept;
private:
  execution_purpose(execution_purpose_kind kind,
                    std::optional<pkgsource::lifecycle_action> action);
  execution_purpose_kind kind_;
  std::optional<pkgsource::lifecycle_action> action_;
};

/*! \brief Canonical absolute path in the process-visible root. */
class PKGEXEC_API logical_path final {
public:
  [[nodiscard]] static logical_path parse(std::string_view value);
  [[nodiscard]] const std::string& string() const noexcept;
  friend PKGEXEC_API bool operator==(const logical_path& lhs,
                         const logical_path& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const logical_path& lhs,
                         const logical_path& rhs) noexcept;
  friend PKGEXEC_API bool operator<(const logical_path& lhs,
                        const logical_path& rhs) noexcept;
private:
  explicit logical_path(std::string value);
  std::string value_;
};

/*! \brief One semantic resource slot. */
class PKGEXEC_API resource_slot final {
public:
  [[nodiscard]] static resource_slot singleton(resource_role role);
  [[nodiscard]] static resource_slot named(resource_role role, std::string name);
  [[nodiscard]] resource_role role() const noexcept;
  [[nodiscard]] const std::string& name() const noexcept;
  [[nodiscard]] std::string text() const;
  friend PKGEXEC_API bool operator==(const resource_slot& lhs,
                         const resource_slot& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const resource_slot& lhs,
                         const resource_slot& rhs) noexcept;
  friend PKGEXEC_API bool operator<(const resource_slot& lhs,
                        const resource_slot& rhs) noexcept;
private:
  resource_slot(resource_role role, std::string name);
  resource_role role_;
  std::string name_;
};

/*! \brief One exact resource made visible at one logical path. */
class PKGEXEC_API resource_binding final {
public:
  resource_binding(resource_slot slot, resource_identity resource,
                   resource_access access, logical_path mount_point);
  [[nodiscard]] const resource_slot& slot() const noexcept;
  [[nodiscard]] const resource_identity& resource() const noexcept;
  [[nodiscard]] resource_access access() const noexcept;
  [[nodiscard]] const logical_path& mount_point() const noexcept;
  friend PKGEXEC_API bool operator==(const resource_binding& lhs,
                         const resource_binding& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const resource_binding& lhs,
                         const resource_binding& rhs) noexcept;
  friend PKGEXEC_API bool operator<(const resource_binding& lhs,
                        const resource_binding& rhs) noexcept;
private:
  resource_slot slot_;
  resource_identity resource_;
  resource_access access_;
  logical_path mount_point_;
};

/*! \brief Complete logical resource layout for one execution. */
class PKGEXEC_API resource_layout final {
public:
  [[nodiscard]] static resource_layout seal(
      std::vector<resource_binding> bindings,
      resource_slot working_directory);
  [[nodiscard]] const std::vector<resource_binding>& bindings() const noexcept;
  [[nodiscard]] const resource_slot& working_directory() const noexcept;
  [[nodiscard]] const resource_layout_identity& identity() const noexcept;
  [[nodiscard]] const resource_binding& binding(const resource_slot& slot) const;
  friend PKGEXEC_API bool operator==(const resource_layout& lhs,
                         const resource_layout& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const resource_layout& lhs,
                         const resource_layout& rhs) noexcept;
private:
  resource_layout(std::vector<resource_binding> bindings,
                  resource_slot working_directory,
                  resource_layout_identity identity);
  std::vector<resource_binding> bindings_;
  resource_slot working_directory_;
  resource_layout_identity identity_;
};

/*! \brief One explicitly admitted environment variable. */
class PKGEXEC_API environment_variable final {
public:
  environment_variable(std::string name, std::string value);
  [[nodiscard]] const std::string& name() const noexcept;
  [[nodiscard]] const std::string& value() const noexcept;
  friend PKGEXEC_API bool operator==(const environment_variable& lhs,
                         const environment_variable& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const environment_variable& lhs,
                         const environment_variable& rhs) noexcept;
  friend PKGEXEC_API bool operator<(const environment_variable& lhs,
                        const environment_variable& rhs) noexcept;
private:
  std::string name_;
  std::string value_;
};

/*! \brief Closed environment and process-I/O policy. */
class PKGEXEC_API environment_policy final {
public:
  [[nodiscard]] static environment_policy hermetic(
      std::vector<logical_path> executable_search_path,
      logical_path home_directory,
      logical_path temporary_directory,
      std::uint32_t parallelism,
      std::uint32_t file_creation_mask = 0022,
      std::optional<std::int64_t> source_date_epoch = std::nullopt,
      network_policy network = network_policy::denied,
      stdin_policy standard_input = stdin_policy::closed,
      stream_policy standard_output = stream_policy::capture_complete,
      stream_policy standard_error = stream_policy::capture_complete,
      std::vector<environment_variable> additional_variables = {});
  [[nodiscard]] locale_policy locale() const noexcept;
  [[nodiscard]] timezone_policy timezone() const noexcept;
  [[nodiscard]] home_policy home() const noexcept;
  [[nodiscard]] network_policy network() const noexcept;
  [[nodiscard]] stdin_policy standard_input() const noexcept;
  [[nodiscard]] stream_policy standard_output() const noexcept;
  [[nodiscard]] stream_policy standard_error() const noexcept;
  [[nodiscard]] const std::vector<logical_path>& executable_search_path() const noexcept;
  [[nodiscard]] const logical_path& home_directory() const noexcept;
  [[nodiscard]] const logical_path& temporary_directory() const noexcept;
  [[nodiscard]] std::uint32_t parallelism() const noexcept;
  [[nodiscard]] std::uint32_t file_creation_mask() const noexcept;
  [[nodiscard]] const std::optional<std::int64_t>& source_date_epoch() const noexcept;
  [[nodiscard]] const std::vector<environment_variable>& additional_variables() const noexcept;
  [[nodiscard]] const environment_policy_identity& identity() const noexcept;
  friend PKGEXEC_API bool operator==(const environment_policy& lhs,
                         const environment_policy& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const environment_policy& lhs,
                         const environment_policy& rhs) noexcept;
private:
  environment_policy(std::vector<logical_path> executable_search_path,
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
                     environment_policy_identity identity);
  std::vector<logical_path> executable_search_path_;
  logical_path home_directory_;
  logical_path temporary_directory_;
  std::uint32_t parallelism_;
  std::uint32_t file_creation_mask_;
  std::optional<std::int64_t> source_date_epoch_;
  network_policy network_;
  stdin_policy standard_input_;
  stream_policy standard_output_;
  stream_policy standard_error_;
  std::vector<environment_variable> additional_variables_;
  environment_policy_identity identity_;
};

/*! \brief Exact numeric credential policy. */
class PKGEXEC_API credential_policy final {
public:
  [[nodiscard]] static credential_policy fixed(
      std::uint64_t user_id,
      std::uint64_t group_id,
      std::vector<std::uint64_t> supplementary_groups = {},
      bool no_new_privileges = true);
  [[nodiscard]] std::uint64_t user_id() const noexcept;
  [[nodiscard]] std::uint64_t group_id() const noexcept;
  [[nodiscard]] const std::vector<std::uint64_t>& supplementary_groups() const noexcept;
  [[nodiscard]] bool no_new_privileges() const noexcept;
  [[nodiscard]] const credential_policy_identity& identity() const noexcept;
  friend PKGEXEC_API bool operator==(const credential_policy& lhs,
                         const credential_policy& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const credential_policy& lhs,
                         const credential_policy& rhs) noexcept;
private:
  credential_policy(std::uint64_t user_id,
                    std::uint64_t group_id,
                    std::vector<std::uint64_t> supplementary_groups,
                    bool no_new_privileges,
                    credential_policy_identity identity);
  std::uint64_t user_id_;
  std::uint64_t group_id_;
  std::vector<std::uint64_t> supplementary_groups_;
  bool no_new_privileges_;
  credential_policy_identity identity_;
};

/*! \brief Optional deterministic resource-limit policy. */
class PKGEXEC_API resource_limits final {
public:
  [[nodiscard]] static resource_limits make(
      std::optional<std::uint64_t> cpu_time_milliseconds = std::nullopt,
      std::optional<std::uint64_t> address_space_bytes = std::nullopt,
      std::optional<std::uint64_t> file_size_bytes = std::nullopt,
      std::optional<std::uint64_t> open_files = std::nullopt,
      std::optional<std::uint64_t> process_count = std::nullopt);
  [[nodiscard]] const std::optional<std::uint64_t>& cpu_time_milliseconds() const noexcept;
  [[nodiscard]] const std::optional<std::uint64_t>& address_space_bytes() const noexcept;
  [[nodiscard]] const std::optional<std::uint64_t>& file_size_bytes() const noexcept;
  [[nodiscard]] const std::optional<std::uint64_t>& open_files() const noexcept;
  [[nodiscard]] const std::optional<std::uint64_t>& process_count() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] const resource_limits_identity& identity() const noexcept;
  friend PKGEXEC_API bool operator==(const resource_limits& lhs,
                         const resource_limits& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const resource_limits& lhs,
                         const resource_limits& rhs) noexcept;
private:
  resource_limits(std::optional<std::uint64_t> cpu_time_milliseconds,
                  std::optional<std::uint64_t> address_space_bytes,
                  std::optional<std::uint64_t> file_size_bytes,
                  std::optional<std::uint64_t> open_files,
                  std::optional<std::uint64_t> process_count,
                  resource_limits_identity identity);
  std::optional<std::uint64_t> cpu_time_milliseconds_;
  std::optional<std::uint64_t> address_space_bytes_;
  std::optional<std::uint64_t> file_size_bytes_;
  std::optional<std::uint64_t> open_files_;
  std::optional<std::uint64_t> process_count_;
  resource_limits_identity identity_;
};

/*! \brief Explicit cancellation behavior. */
class PKGEXEC_API cancellation_policy final {
public:
  [[nodiscard]] static cancellation_policy disabled();
  [[nodiscard]] static cancellation_policy graceful_then_forced(
      std::uint64_t grace_period_milliseconds);
  [[nodiscard]] cancellation_mode mode() const noexcept;
  [[nodiscard]] const std::optional<std::uint64_t>& grace_period_milliseconds() const noexcept;
  friend PKGEXEC_API bool operator==(const cancellation_policy& lhs,
                         const cancellation_policy& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const cancellation_policy& lhs,
                         const cancellation_policy& rhs) noexcept;
private:
  cancellation_policy(cancellation_mode mode,
                      std::optional<std::uint64_t> grace_period_milliseconds);
  cancellation_mode mode_;
  std::optional<std::uint64_t> grace_period_milliseconds_;
};

/*! \brief Complete process termination observation. */
class PKGEXEC_API process_termination final {
public:
  [[nodiscard]] static process_termination exited(std::uint32_t status);
  [[nodiscard]] static process_termination signaled(std::uint32_t signal);
  [[nodiscard]] static process_termination cancelled();
  [[nodiscard]] static process_termination resource_limited(resource_limit_kind limit);
  [[nodiscard]] process_termination_kind kind() const noexcept;
  [[nodiscard]] const std::optional<std::uint32_t>& value() const noexcept;
  [[nodiscard]] const std::optional<resource_limit_kind>& limit() const noexcept;
  friend PKGEXEC_API bool operator==(const process_termination& lhs,
                         const process_termination& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const process_termination& lhs,
                         const process_termination& rhs) noexcept;
private:
  process_termination(process_termination_kind kind,
                      std::optional<std::uint32_t> value,
                      std::optional<resource_limit_kind> limit);
  process_termination_kind kind_;
  std::optional<std::uint32_t> value_;
  std::optional<resource_limit_kind> limit_;
};

/*! \brief Complete or digest-only captured stream evidence. */
class PKGEXEC_API stream_capture final {
public:
  [[nodiscard]] static stream_capture retained(std::string material);
  [[nodiscard]] static stream_capture observed(std::uint64_t byte_count,
                                               sha256_digest digest);
  [[nodiscard]] std::uint64_t byte_count() const noexcept;
  [[nodiscard]] const sha256_digest& digest() const noexcept;
  [[nodiscard]] const std::optional<std::string>& material() const noexcept;
  friend PKGEXEC_API bool operator==(const stream_capture& lhs,
                         const stream_capture& rhs) noexcept;
  friend PKGEXEC_API bool operator!=(const stream_capture& lhs,
                         const stream_capture& rhs) noexcept;
private:
  stream_capture(std::uint64_t byte_count,
                 sha256_digest digest,
                 std::optional<std::string> material);
  std::uint64_t byte_count_;
  sha256_digest digest_;
  std::optional<std::string> material_;
};

} // namespace pkgexec
