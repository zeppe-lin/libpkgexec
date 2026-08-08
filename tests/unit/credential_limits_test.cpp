// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <vector>

int main()
{
  using namespace pkgexec;

  const auto credentials = credential_policy::fixed(1000, 1000, {44, 27});
  const auto reordered = credential_policy::fixed(1000, 1000, {27, 44});
  TEST_CHECK(credentials == reordered);
  TEST_CHECK(credentials.supplementary_groups() ==
             std::vector<std::uint64_t>({27, 44}));
  TEST_CHECK(credentials.no_new_privileges());
  TEST_CHECK(credential_policy::fixed(1000, 1000, {}, false).identity() !=
             credentials.identity());
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   credential_policy::fixed(1000, 1000, {1000}));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   credential_policy::fixed(1000, 1000, {27, 27}));

  TEST_CHECK(resource_limits::make().empty());
  const auto limits = resource_limits::make(1, 2, 3, 4, 5);
  TEST_CHECK(limits.cpu_time_milliseconds() == 1U);
  TEST_CHECK(limits.address_space_bytes() == 2U);
  TEST_CHECK(limits.file_size_bytes() == 3U);
  TEST_CHECK(limits.open_files() == 4U);
  TEST_CHECK(limits.process_count() == 5U);
  TEST_EXEC_THROWS(error_code::invalid_policy, resource_limits::make(0));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   resource_limits::make(std::nullopt, 0));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   resource_limits::make(std::nullopt, std::nullopt, 0));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   resource_limits::make(std::nullopt, std::nullopt,
                                         std::nullopt, 0));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   resource_limits::make(std::nullopt, std::nullopt,
                                         std::nullopt, std::nullopt, 0));

  TEST_CHECK(cancellation_policy::disabled().mode() == cancellation_mode::disabled);
  TEST_CHECK(!cancellation_policy::disabled().grace_period_milliseconds());
  TEST_CHECK(cancellation_policy::graceful_then_forced(1)
                 .grace_period_milliseconds() == 1U);
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   cancellation_policy::graceful_then_forced(0));

  TEST_CHECK(process_termination::exited(255).value() == 255U);
  TEST_EXEC_THROWS(error_code::invalid_value,
                   process_termination::exited(256));
  TEST_CHECK(process_termination::signaled(15).value() == 15U);
  TEST_EXEC_THROWS(error_code::invalid_value,
                   process_termination::signaled(0));
  TEST_CHECK(process_termination::cancelled().kind() ==
             process_termination_kind::cancelled);
  TEST_CHECK(process_termination::resource_limited(resource_limit_kind::open_files)
                 .limit() == resource_limit_kind::open_files);

  const auto retained = stream_capture::retained(std::string("a\0b", 3));
  TEST_CHECK(retained.byte_count() == 3U);
  TEST_CHECK(retained.material()->size() == 3U);
  const auto observed = stream_capture::observed(3, retained.digest());
  TEST_CHECK(observed.byte_count() == 3U);
  TEST_CHECK(!observed.material());
  TEST_CHECK(observed != retained);

  return EXIT_SUCCESS;
}
