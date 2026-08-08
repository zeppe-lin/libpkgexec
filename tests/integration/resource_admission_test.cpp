// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>
#include <filesystem>

int main()
{
  using namespace pkgexec;

  TEST_EXEC_THROWS(error_code::invalid_path,
                   resource_materialization(fixture::resource("x"), "relative"));
  const auto normalized = resource_materialization(
      fixture::resource("x"), "/host/a/../b");
  TEST_CHECK(normalized.host_path() == std::filesystem::path("/host/b"));

  const auto request = fixture::uncontrolled_request();
  auto materializations = fixture::materializations();
  std::reverse(materializations.begin(), materializations.end());
  const auto resources = execution_resources::admit(
      request, request.root_view(), "/host/root/./", std::move(materializations));
  TEST_CHECK(resources.root_view() == request.root_view());
  TEST_CHECK(resources.root_view_path() == std::filesystem::path("/host/root/"));
  TEST_CHECK(resources.materializations().size() == 4U);
  TEST_CHECK(resources.materialization(fixture::resource("source-main"))
                 .host_path() == "/host/source");

  TEST_EXEC_THROWS(error_code::resource_mismatch,
      execution_resources::admit(request, fixture::root("wrong"), "/host/root",
                                 fixture::materializations()));
  TEST_EXEC_THROWS(error_code::invalid_path,
      execution_resources::admit(request, request.root_view(), "relative",
                                 fixture::materializations()));

  auto missing = fixture::materializations();
  missing.pop_back();
  TEST_EXEC_THROWS(error_code::resource_mismatch,
      execution_resources::admit(request, request.root_view(), "/host/root",
                                 std::move(missing)));

  auto duplicate = fixture::materializations();
  duplicate.push_back(resource_materialization(
      fixture::resource("workspace"), "/other"));
  TEST_EXEC_THROWS(error_code::duplicate_resource,
      execution_resources::admit(request, request.root_view(), "/host/root",
                                 std::move(duplicate)));

  auto foreign = fixture::materializations();
  foreign.back() = resource_materialization(fixture::resource("foreign"),
                                            "/host/foreign");
  TEST_EXEC_THROWS(error_code::resource_mismatch,
      execution_resources::admit(request, request.root_view(), "/host/root",
                                 std::move(foreign)));
  TEST_EXEC_THROWS(error_code::resource_mismatch,
                   resources.materialization(fixture::resource("foreign")));
  return EXIT_SUCCESS;
}
