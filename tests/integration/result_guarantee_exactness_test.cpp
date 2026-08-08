// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/guarantees.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;

  const auto base = fixture::uncontrolled_request();
  const auto allowed_environment = environment_policy::hermetic(
      {logical_path::parse("/bin")}, logical_path::parse("/home/build"),
      logical_path::parse("/tmp"), 1, 0022, std::nullopt,
      network_policy::allowed, stdin_policy::closed,
      stream_policy::capture_complete, stream_policy::capture_complete);
  const auto request = execution_request::seal(
      base.program(), base.purpose(), base.interpreter(), base.root_view(),
      base.resources(), allowed_environment, base.credentials(),
      resource_limits::make(), cancellation_policy::disabled());

  auto profile_guarantees = test_support::with_guarantee(
      request.required_guarantees(), execution_guarantee::network_denied);
  profile_guarantees = test_support::with_guarantee(
      std::move(profile_guarantees), execution_guarantee::cancellation);
  const auto profile = backend_capability_profile::seal(
      fixture::backend("over-capable"), profile_guarantees);
  TEST_CHECK(profile.supports(request));

  const auto output = stream_capture::retained("");
  const auto error_output = stream_capture::retained("");
  const auto network_overclaim = test_support::with_guarantee(
      request.required_guarantees(), execution_guarantee::network_denied);
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::succeeded(
          request, profile, request.interpreter(), output, error_output,
          network_overclaim));
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::process_start_failed,
          {execution_guarantee::network_denied}));

  const auto cancellation_overclaim = test_support::with_guarantee(
      request.required_guarantees(), execution_guarantee::cancellation);
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(1),
          output, error_output, cancellation_overclaim,
          cleanup_outcome::verified,
          execution_failure_kind::program_exited_nonzero));

  const auto exact = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      request.required_guarantees());
  TEST_CHECK(exact.established_guarantees() == request.required_guarantees());
  return EXIT_SUCCESS;
}
