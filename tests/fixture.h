// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgexec/libpkgexec.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace fixture {

inline std::string digest(std::string_view value)
{
  return pkgexec::sha256_digest::of_bytes(value).hex();
}

inline pkgexec::resource_identity resource(std::string_view value)
{
  return pkgexec::resource_identity::from_sha256(digest(value));
}
inline pkgexec::root_view_identity root(std::string_view value = "root")
{
  return pkgexec::root_view_identity::from_sha256(digest(value));
}
inline pkgexec::interpreter_identity interpreter(
    std::string_view value = "posix-shell")
{
  return pkgexec::interpreter_identity::from_sha256(digest(value));
}
inline pkgexec::backend_identity backend(std::string_view value = "test-backend")
{
  return pkgexec::backend_identity::from_sha256(digest(value));
}

inline pkgexec::resource_layout layout(bool reverse = false)
{
  using namespace pkgexec;
  std::vector<resource_binding> bindings;
  bindings.emplace_back(
      resource_slot::named(resource_role::source_tree, "main"),
      resource("source-main"), resource_access::read_only,
      logical_path::parse("/src"));
  bindings.emplace_back(
      resource_slot::singleton(resource_role::build_workspace),
      resource("workspace"), resource_access::writable,
      logical_path::parse("/build"));
  bindings.emplace_back(
      resource_slot::singleton(resource_role::package_output_root),
      resource("package-root"), resource_access::writable,
      logical_path::parse("/pkg"));
  bindings.emplace_back(
      resource_slot::singleton(resource_role::private_temporary_root),
      resource("temporary"), resource_access::writable,
      logical_path::parse("/tmp"));
  if (reverse) {
    std::reverse(bindings.begin(), bindings.end());
  }
  return resource_layout::seal(
      std::move(bindings),
      resource_slot::singleton(resource_role::build_workspace));
}

inline pkgexec::environment_policy environment(bool reverse_variables = false)
{
  using namespace pkgexec;
  std::vector<environment_variable> variables{
      environment_variable("ARFLAGS", "rcD"),
      environment_variable("MAKEFLAGS", "-j4"),
  };
  if (reverse_variables) {
    std::reverse(variables.begin(), variables.end());
  }
  return environment_policy::hermetic(
      {logical_path::parse("/usr/bin"), logical_path::parse("/bin")},
      logical_path::parse("/home/build"), logical_path::parse("/tmp"), 4,
      0022, 1, network_policy::denied, stdin_policy::closed,
      stream_policy::capture_complete, stream_policy::capture_complete,
      std::move(variables));
}

inline pkgexec::resource_limits default_limits()
{
  return pkgexec::resource_limits::make(
      1000, 1024 * 1024, std::nullopt, 128, 64);
}

inline pkgexec::execution_request request_with_limits(
    pkgexec::resource_limits limits,
    pkgexec::cancellation_policy cancellation,
    bool reverse = false)
{
  using namespace pkgexec;
  return execution_request::seal(
      pkgsource::program(pkgsource::program_language::posix_shell,
                         "printf '%s\\n' build"),
      execution_purpose::build(), interpreter(), root(), layout(reverse),
      environment(reverse), credential_policy::fixed(1000, 1000, {27, 44}),
      std::move(limits), std::move(cancellation));
}

inline pkgexec::execution_request request_with_cancellation(
    pkgexec::cancellation_policy cancellation,
    bool reverse = false)
{
  return request_with_limits(
      default_limits(), std::move(cancellation), reverse);
}

inline pkgexec::execution_request request(bool reverse = false)
{
  return request_with_cancellation(
      pkgexec::cancellation_policy::graceful_then_forced(500), reverse);
}

inline pkgexec::execution_request uncontrolled_request(bool reverse = false)
{
  return request_with_cancellation(
      pkgexec::cancellation_policy::disabled(), reverse);
}

inline pkgexec::backend_capability_profile profile(
    const pkgexec::execution_request& value)
{
  return pkgexec::backend_capability_profile::seal(
      backend(), value.required_guarantees());
}

inline std::vector<pkgexec::resource_materialization> materializations()
{
  using namespace pkgexec;
  return {
      resource_materialization(resource("source-main"), "/host/source"),
      resource_materialization(resource("workspace"), "/host/workspace"),
      resource_materialization(resource("package-root"), "/host/package"),
      resource_materialization(resource("temporary"), "/host/temporary"),
  };
}

} // namespace fixture
