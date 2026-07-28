// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

namespace {

bool has(const std::vector<pkgexec::execution_guarantee>& values,
         pkgexec::execution_guarantee value)
{
  return std::binary_search(values.begin(), values.end(), value);
}

} // namespace

int main()
{
  using namespace pkgexec;

  const auto first = fixture::request(false);
  const auto reordered = fixture::request(true);
  TEST_CHECK(first.identity() == reordered.identity());
  TEST_CHECK(first.schema_version() == execution_request_schema_version);
  TEST_CHECK(first.purpose().kind() == execution_purpose_kind::build);
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::exact_interpreter));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::closed_environment));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::read_only_resources));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::writable_resources));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::network_denied));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::resource_limits));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::cpu_time_limit));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::address_space_limit));
  TEST_CHECK(!has(first.required_guarantees(), execution_guarantee::file_size_limit));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::open_files_limit));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::process_count_limit));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::cancellation));
  TEST_CHECK(has(first.required_guarantees(), execution_guarantee::cleanup_verified));

  auto check = execution_request::seal(
      first.program(), execution_purpose::check(), first.interpreter(),
      first.root_view(), first.resources(), first.environment(),
      first.credentials(), first.limits(), first.cancellation());
  TEST_CHECK(check.identity() != first.identity());

  auto lifecycle = execution_request::seal(
      first.program(),
      execution_purpose::lifecycle(pkgsource::lifecycle_action::post_install),
      first.interpreter(), first.root_view(), first.resources(),
      first.environment(), first.credentials(), first.limits(),
      first.cancellation());
  TEST_CHECK(lifecycle.identity() != check.identity());

  auto different_program = execution_request::seal(
      pkgsource::program(pkgsource::program_language::posix_shell, "true"),
      first.purpose(), first.interpreter(), first.root_view(),
      first.resources(), first.environment(), first.credentials(),
      first.limits(), first.cancellation());
  TEST_CHECK(different_program.identity() != first.identity());

  auto different_interpreter = execution_request::seal(
      first.program(), first.purpose(), fixture::interpreter("other"),
      first.root_view(), first.resources(), first.environment(),
      first.credentials(), first.limits(), first.cancellation());
  TEST_CHECK(different_interpreter.identity() != first.identity());

  const auto allowed_network = environment_policy::hermetic(
      {logical_path::parse("/bin")}, logical_path::parse("/home/build"),
      logical_path::parse("/tmp"), 1, 0022, std::nullopt,
      network_policy::allowed);
  TEST_EXEC_THROWS(error_code::invalid_policy,
      execution_request::seal(
          first.program(), first.purpose(), first.interpreter(),
          first.root_view(), first.resources(), allowed_network,
          first.credentials(), resource_limits::make(),
          cancellation_policy::disabled(),
          {execution_guarantee::network_denied}));

  const auto no_limits = fixture::request_with_limits(
      resource_limits::make(), cancellation_policy::disabled());
  TEST_CHECK(!has(no_limits.required_guarantees(),
                  execution_guarantee::resource_limits));
  TEST_CHECK(!has(no_limits.required_guarantees(),
                  execution_guarantee::address_space_limit));

  const auto address_only = fixture::request_with_limits(
      resource_limits::make(std::nullopt, 4096),
      cancellation_policy::disabled());
  TEST_CHECK(has(address_only.required_guarantees(),
                 execution_guarantee::resource_limits));
  TEST_CHECK(has(address_only.required_guarantees(),
                 execution_guarantee::address_space_limit));
  TEST_CHECK(!has(address_only.required_guarantees(),
                  execution_guarantee::cpu_time_limit));
  auto file_only = no_limits.required_guarantees();
  file_only.push_back(execution_guarantee::resource_limits);
  file_only.push_back(execution_guarantee::file_size_limit);
  TEST_CHECK(!backend_capability_profile::seal(
      fixture::backend("file-only"), file_only).supports(address_only));

  TEST_EXEC_THROWS(error_code::invalid_policy,
      execution_request::seal(
          no_limits.program(), no_limits.purpose(), no_limits.interpreter(),
          no_limits.root_view(), no_limits.resources(), no_limits.environment(),
          no_limits.credentials(), no_limits.limits(), no_limits.cancellation(),
          {execution_guarantee::file_size_limit}));

  return EXIT_SUCCESS;
}
