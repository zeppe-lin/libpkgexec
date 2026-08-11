// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;

  const auto first = fixture::environment(false);
  const auto reordered_variables = fixture::environment(true);
  TEST_CHECK(first == reordered_variables);
  TEST_CHECK(first.identity() == reordered_variables.identity());
  TEST_CHECK(first.additional_variables().front().name() == "ARFLAGS");
  TEST_CHECK(first.locale() == locale_policy::c_utf8);
  TEST_CHECK(first.timezone() == timezone_policy::utc);
  TEST_CHECK(first.home() == home_policy::isolated);
  TEST_CHECK(first.parallelism() == 4U);
  TEST_CHECK(first.file_creation_mask() == 0022U);
  TEST_CHECK(first.source_date_epoch() == 1);

  const auto path_reordered = environment_policy::hermetic(
      {logical_path::parse("/bin"), logical_path::parse("/usr/bin")},
      first.home_directory(), first.temporary_directory(), first.parallelism(),
      first.file_creation_mask(), first.source_date_epoch(), first.network(),
      first.standard_input(), first.standard_output(), first.standard_error(),
      first.additional_variables());
  TEST_CHECK(path_reordered.identity() != first.identity());

  TEST_EXEC_THROWS(error_code::reserved_environment_variable,
                   environment_variable("PATH", "/bad"));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   environment_variable("1BAD", "x"));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   environment_variable("BAD-NAME", "x"));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   environment_variable("BAD", std::string("a\0b", 3)));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({}, logical_path::parse("/home"),
          logical_path::parse("/tmp"), 1));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 0));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1, 01000));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1,
          0022, -1));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic(
          {logical_path::parse("/bin"), logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1));
  TEST_EXEC_THROWS(error_code::duplicate_environment_variable,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1,
          0022, std::nullopt, network_policy::denied, stdin_policy::closed,
          stream_policy::capture_complete, stream_policy::capture_complete,
          {environment_variable("FOO", "a"), environment_variable("FOO", "b")}));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1,
          0022, std::nullopt, static_cast<network_policy>(255)));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1,
          0022, std::nullopt, network_policy::denied,
          static_cast<stdin_policy>(255)));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic({logical_path::parse("/bin")},
          logical_path::parse("/home"), logical_path::parse("/tmp"), 1,
          0022, std::nullopt, network_policy::denied, stdin_policy::closed,
          static_cast<stream_policy>(255)));

  return EXIT_SUCCESS;
}
