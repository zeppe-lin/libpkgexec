// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;

  static_assert(static_cast<int>(execution_guarantee::resource_limits) == 8);
  static_assert(static_cast<int>(execution_guarantee::cancellation) == 9);
  static_assert(static_cast<int>(execution_guarantee::cleanup_verified) == 12);
  static_assert(static_cast<int>(execution_guarantee::cpu_time_limit) == 13);
  static_assert(static_cast<int>(execution_guarantee::process_count_limit) == 17);

  TEST_CHECK(execution_purpose::build().kind() == execution_purpose_kind::build);
  TEST_CHECK(!execution_purpose::check().action());
  TEST_CHECK(execution_purpose::lifecycle(
      pkgsource::lifecycle_action::post_install).action() ==
      pkgsource::lifecycle_action::post_install);

  TEST_CHECK(logical_path::parse("/").string() == "/");
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("relative"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a//b"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a/../b"));
  TEST_EXEC_THROWS(error_code::invalid_path, logical_path::parse("/a/"));

  const auto first_layout = fixture::layout(false);
  const auto second_layout = fixture::layout(true);
  TEST_CHECK(first_layout.identity() == second_layout.identity());
  TEST_CHECK(first_layout.bindings() == second_layout.bindings());
  TEST_CHECK(first_layout.binding(resource_slot::singleton(
      resource_role::build_workspace)).mount_point().string() == "/build");

  TEST_EXEC_THROWS(error_code::invalid_policy,
      resource_binding(
          resource_slot::named(resource_role::source_tree, "main"),
          fixture::resource("source"), resource_access::writable,
          logical_path::parse("/src")));
  TEST_EXEC_THROWS(error_code::invalid_value,
      resource_slot::singleton(resource_role::source_tree));
  TEST_EXEC_THROWS(error_code::invalid_value,
      resource_slot::named(resource_role::build_workspace, "bad"));

  std::vector<resource_binding> duplicate_slots{
      resource_binding(resource_slot::singleton(resource_role::build_workspace),
                       fixture::resource("a"), resource_access::writable,
                       logical_path::parse("/a")),
      resource_binding(resource_slot::singleton(resource_role::build_workspace),
                       fixture::resource("b"), resource_access::writable,
                       logical_path::parse("/b")),
  };
  TEST_EXEC_THROWS(error_code::duplicate_resource,
      resource_layout::seal(
          std::move(duplicate_slots),
          resource_slot::singleton(resource_role::build_workspace)));

  const auto first_environment = fixture::environment(false);
  const auto second_environment = fixture::environment(true);
  TEST_CHECK(first_environment.identity() == second_environment.identity());
  TEST_CHECK(first_environment.additional_variables().front().name() == "ARFLAGS");
  TEST_EXEC_THROWS(error_code::reserved_environment_variable,
                   environment_variable("PATH", "/bad"));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic(
          {}, logical_path::parse("/home"), logical_path::parse("/tmp"), 1));
  TEST_EXEC_THROWS(error_code::invalid_policy,
      environment_policy::hermetic(
          {logical_path::parse("/bin")}, logical_path::parse("/home"),
          logical_path::parse("/tmp"), 0));
  TEST_EXEC_THROWS(error_code::duplicate_environment_variable,
      environment_policy::hermetic(
          {logical_path::parse("/bin")}, logical_path::parse("/home"),
          logical_path::parse("/tmp"), 1, 0022, std::nullopt,
          network_policy::denied, stdin_policy::closed,
          stream_policy::capture_complete, stream_policy::capture_complete,
          {environment_variable("FOO", "a"),
           environment_variable("FOO", "b")}));

  const auto credentials = credential_policy::fixed(1000, 1000, {44, 27});
  TEST_CHECK(credentials.supplementary_groups() ==
             std::vector<std::uint64_t>({27, 44}));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   credential_policy::fixed(1000, 1000, {1000}));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   credential_policy::fixed(1000, 1000, {27, 27}));

  TEST_CHECK(to_string(execution_guarantee::cpu_time_limit) ==
             "cpu-time-limit");
  TEST_CHECK(to_string(execution_guarantee::address_space_limit) ==
             "address-space-limit");
  TEST_CHECK(to_string(execution_guarantee::file_size_limit) ==
             "file-size-limit");
  TEST_CHECK(to_string(execution_guarantee::open_files_limit) ==
             "open-files-limit");
  TEST_CHECK(to_string(execution_guarantee::process_count_limit) ==
             "process-count-limit");

  TEST_CHECK(resource_limits::make().empty());
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   resource_limits::make(0));
  TEST_EXEC_THROWS(error_code::invalid_policy,
                   cancellation_policy::graceful_then_forced(0));
  TEST_EXEC_THROWS(error_code::invalid_value,
                   process_termination::exited(256));
  TEST_EXEC_THROWS(error_code::invalid_value,
                   process_termination::signaled(0));

  const auto capture = stream_capture::retained(std::string("a\0b", 3));
  TEST_CHECK(capture.byte_count() == 3);
  TEST_CHECK(capture.material()->size() == 3);
  const auto observed = stream_capture::observed(2, sha256_digest::of_bytes("a"));
  TEST_CHECK(observed.byte_count() == 2);
  TEST_CHECK(!observed.material());

  return EXIT_SUCCESS;
}
