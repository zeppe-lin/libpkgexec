// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <string>

int main()
{
  using namespace pkgexec;

  TEST_CHECK(execution_purpose::build().kind() == execution_purpose_kind::build);
  TEST_CHECK(!execution_purpose::build().action());
  TEST_CHECK(execution_purpose::check().kind() == execution_purpose_kind::check);
  TEST_CHECK(!execution_purpose::check().action());
  for (const auto action : {
           pkgsource::lifecycle_action::pre_install,
           pkgsource::lifecycle_action::post_install,
           pkgsource::lifecycle_action::pre_remove,
           pkgsource::lifecycle_action::post_remove,
       }) {
    const auto purpose = execution_purpose::lifecycle(action);
    TEST_CHECK(purpose.kind() == execution_purpose_kind::lifecycle);
    TEST_CHECK(purpose.action() == action);
  }

  TEST_CHECK(logical_path::parse("/").string() == "/");
  TEST_CHECK(logical_path::parse("/a/b").string() == "/a/b");
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse(""));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("relative"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a//b"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a/./b"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a/../b"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a/"));
  TEST_EXEC_THROWS(error_code::invalid_path,
                   logical_path::parse(std::string("/a\0b", 4)));

  TEST_CHECK(to_string(execution_purpose_kind::lifecycle) == "lifecycle");
  TEST_CHECK(to_string(network_policy::loopback_only) == "loopback-only");
  TEST_CHECK(to_string(execution_failure_kind::cleanup_failed) == "cleanup-failed");
  return EXIT_SUCCESS;
}
