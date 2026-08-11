// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>
#include <vector>

int main()
{
  using namespace pkgexec;

  TEST_CHECK(resource_slot::named(resource_role::source_tree, "main").text() ==
             "source-tree:main");
  TEST_CHECK(resource_slot::singleton(resource_role::build_workspace).text() ==
             "build-workspace");
  TEST_EXEC_THROWS(error_code::invalid_value,
                   resource_slot::singleton(resource_role::source_tree));
  TEST_EXEC_THROWS(error_code::invalid_value,
                   resource_slot::named(resource_role::build_workspace, "bad"));
  TEST_EXEC_THROWS(error_code::invalid_value,
                   resource_slot::named(resource_role::source_tree, "-bad"));
  TEST_EXEC_THROWS(error_code::invalid_value,
                   resource_slot::named(resource_role::source_tree, "bad/name"));
  TEST_EXEC_THROWS(error_code::invalid_value,
      resource_slot::singleton(static_cast<resource_role>(255)));
  TEST_EXEC_THROWS(error_code::invalid_value,
      resource_slot::named(static_cast<resource_role>(255), "bad"));

  TEST_EXEC_THROWS(error_code::invalid_policy,
      resource_binding(resource_slot::named(resource_role::source_tree, "main"),
                       fixture::resource("source"), resource_access::writable,
                       logical_path::parse("/src")));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      resource_binding(resource_slot::singleton(resource_role::build_workspace),
                       fixture::resource("workspace"), resource_access::read_only,
                       logical_path::parse("/build")));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      resource_binding(resource_slot::singleton(resource_role::managed_target_root),
                       fixture::resource("target"),
                       static_cast<resource_access>(255),
                       logical_path::parse("/target")));
  TEST_EXEC_THROWS(error_code::invalid_value,
      process_termination::resource_limited(
          static_cast<resource_limit_kind>(255)));

  const auto first = fixture::layout(false);
  const auto reordered = fixture::layout(true);
  TEST_CHECK(first == reordered);
  TEST_CHECK(first.identity() == reordered.identity());
  TEST_CHECK(first.bindings() == reordered.bindings());
  TEST_CHECK(first.binding(resource_slot::singleton(
      resource_role::build_workspace)).mount_point().string() == "/build");
  TEST_EXEC_THROWS(error_code::invalid_value,
      first.binding(resource_slot::named(resource_role::source_tree, "missing")));

  std::vector<resource_binding> duplicate_slots{
      resource_binding(resource_slot::singleton(resource_role::build_workspace),
                       fixture::resource("a"), resource_access::writable,
                       logical_path::parse("/a")),
      resource_binding(resource_slot::singleton(resource_role::build_workspace),
                       fixture::resource("b"), resource_access::writable,
                       logical_path::parse("/b")),
  };
  TEST_EXEC_THROWS(error_code::duplicate_resource,
      resource_layout::seal(std::move(duplicate_slots),
          resource_slot::singleton(resource_role::build_workspace)));

  std::vector<resource_binding> duplicate_mounts{
      resource_binding(resource_slot::singleton(resource_role::build_workspace),
                       fixture::resource("a"), resource_access::writable,
                       logical_path::parse("/same")),
      resource_binding(resource_slot::singleton(resource_role::package_output_root),
                       fixture::resource("b"), resource_access::writable,
                       logical_path::parse("/same")),
  };
  TEST_EXEC_THROWS(error_code::duplicate_mount_point,
      resource_layout::seal(std::move(duplicate_mounts),
          resource_slot::singleton(resource_role::build_workspace)));

  std::vector<resource_binding> missing_working{
      resource_binding(resource_slot::singleton(resource_role::package_output_root),
                       fixture::resource("a"), resource_access::writable,
                       logical_path::parse("/pkg")),
  };
  TEST_EXEC_THROWS(error_code::missing_working_directory,
      resource_layout::seal(std::move(missing_working),
          resource_slot::singleton(resource_role::build_workspace)));
  TEST_EXEC_THROWS(error_code::missing_working_directory,
      resource_layout::seal({},
          resource_slot::singleton(resource_role::build_workspace)));

  return EXIT_SUCCESS;
}
