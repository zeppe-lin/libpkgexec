// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/guarantees.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;
  using test_support::has_guarantee;

  const auto first = fixture::request(false);
  const auto reordered = fixture::request(true);
  TEST_CHECK(first == reordered);
  TEST_CHECK(first.schema_version() == execution_request_schema_version);
  TEST_CHECK(first.program().content_digest() == reordered.program().content_digest());
  TEST_CHECK(first.purpose().kind() == execution_purpose_kind::build);
  for (const auto guarantee : {
           execution_guarantee::exact_interpreter,
           execution_guarantee::closed_environment,
           execution_guarantee::root_view,
           execution_guarantee::read_only_resources,
           execution_guarantee::writable_resources,
           execution_guarantee::fixed_credentials,
           execution_guarantee::network_denied,
           execution_guarantee::resource_limits,
           execution_guarantee::cpu_time_limit,
           execution_guarantee::address_space_limit,
           execution_guarantee::open_files_limit,
           execution_guarantee::process_count_limit,
           execution_guarantee::cancellation,
           execution_guarantee::complete_stdout_capture,
           execution_guarantee::complete_stderr_capture,
           execution_guarantee::cleanup_verified,
       }) {
    TEST_CHECK(has_guarantee(first.required_guarantees(), guarantee));
  }
  TEST_CHECK(!has_guarantee(first.required_guarantees(),
                            execution_guarantee::file_size_limit));

  const auto check = execution_request::seal(
      first.program(), execution_purpose::check(), first.interpreter(),
      first.root_view(), first.resources(), first.environment(),
      first.credentials(), first.limits(), first.cancellation());
  TEST_CHECK(check.identity() != first.identity());

  const auto lifecycle = execution_request::seal(
      first.program(),
      execution_purpose::lifecycle(pkgsource::lifecycle_action::post_install),
      first.interpreter(), first.root_view(), first.resources(),
      first.environment(), first.credentials(), first.limits(),
      first.cancellation());
  TEST_CHECK(lifecycle.identity() != first.identity());
  TEST_CHECK(lifecycle.identity() != check.identity());

  const auto different_program = execution_request::seal(
      pkgsource::program(pkgsource::program_language::posix_shell, "true"),
      first.purpose(), first.interpreter(), first.root_view(), first.resources(),
      first.environment(), first.credentials(), first.limits(),
      first.cancellation());
  TEST_CHECK(different_program.identity() != first.identity());

  const auto different_interpreter = execution_request::seal(
      first.program(), first.purpose(), fixture::interpreter("other"),
      first.root_view(), first.resources(), first.environment(),
      first.credentials(), first.limits(), first.cancellation());
  TEST_CHECK(different_interpreter.identity() != first.identity());

  const auto no_limits = fixture::request_with_limits(
      resource_limits::make(), cancellation_policy::disabled());
  TEST_CHECK(!has_guarantee(no_limits.required_guarantees(),
                            execution_guarantee::resource_limits));
  TEST_CHECK(!has_guarantee(no_limits.required_guarantees(),
                            execution_guarantee::cancellation));

  const auto address_only = fixture::request_with_limits(
      resource_limits::make(std::nullopt, 4096),
      cancellation_policy::disabled());
  TEST_CHECK(has_guarantee(address_only.required_guarantees(),
                           execution_guarantee::resource_limits));
  TEST_CHECK(has_guarantee(address_only.required_guarantees(),
                           execution_guarantee::address_space_limit));
  TEST_CHECK(!has_guarantee(address_only.required_guarantees(),
                            execution_guarantee::cpu_time_limit));

  const auto allowed_network = environment_policy::hermetic(
      {logical_path::parse("/bin")}, logical_path::parse("/home/build"),
      logical_path::parse("/tmp"), 1, 0022, std::nullopt,
      network_policy::allowed, stdin_policy::closed,
      stream_policy::discard, stream_policy::discard);
  const auto allowed = execution_request::seal(
      first.program(), first.purpose(), first.interpreter(), first.root_view(),
      first.resources(), allowed_network, first.credentials(),
      resource_limits::make(), cancellation_policy::disabled());
  TEST_CHECK(!has_guarantee(allowed.required_guarantees(),
                            execution_guarantee::network_denied));
  TEST_CHECK(!has_guarantee(allowed.required_guarantees(),
                            execution_guarantee::loopback_isolated));
  TEST_CHECK(!has_guarantee(allowed.required_guarantees(),
                            execution_guarantee::complete_stdout_capture));
  TEST_CHECK(!has_guarantee(allowed.required_guarantees(),
                            execution_guarantee::complete_stderr_capture));

  const auto loopback_environment = environment_policy::hermetic(
      {logical_path::parse("/bin")}, logical_path::parse("/home/build"),
      logical_path::parse("/tmp"), 1, 0022, std::nullopt,
      network_policy::loopback_only);
  const auto loopback = execution_request::seal(
      first.program(), first.purpose(), first.interpreter(), first.root_view(),
      first.resources(), loopback_environment, first.credentials(),
      resource_limits::make(), cancellation_policy::disabled());
  TEST_CHECK(has_guarantee(loopback.required_guarantees(),
                           execution_guarantee::loopback_isolated));
  TEST_CHECK(!has_guarantee(loopback.required_guarantees(),
                            execution_guarantee::network_denied));

  TEST_EXEC_THROWS(error_code::invalid_policy,
      execution_request::seal(
          first.program(), first.purpose(), first.interpreter(), first.root_view(),
          first.resources(), allowed_network, first.credentials(),
          resource_limits::make(), cancellation_policy::disabled(),
          {execution_guarantee::network_denied}));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      execution_request::seal(
          first.program(), first.purpose(), first.interpreter(), first.root_view(),
          first.resources(), allowed_network, first.credentials(),
          resource_limits::make(), cancellation_policy::disabled(),
          {execution_guarantee::cancellation}));
  return EXIT_SUCCESS;
}
