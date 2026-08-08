// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;

  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  const auto output = stream_capture::retained("stdout\n");
  const auto error_output = stream_capture::retained("stderr\n");
  auto cancellation = cancellation_source::for_request(request);
  const auto token = cancellation.token();

  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::cancelled_before_start(
          request, profile, token, {execution_guarantee::cancellation}));
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::cancelled_after_start(
          request, profile, token, request.interpreter(), output, error_output,
          request.required_guarantees(), cleanup_outcome::verified));

  TEST_CHECK(cancellation.request_cancellation());
  const auto before = execution_result::cancelled_before_start(
      request, profile, token, {execution_guarantee::cancellation},
      "cancelled before start");
  TEST_CHECK(before.start_state() == execution_start_state::not_started);
  TEST_CHECK(!before.termination());
  TEST_CHECK(before.failure() == execution_failure_kind::cancelled);
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::cancelled_before_start(request, profile, token, {}));

  const auto after = execution_result::cancelled_after_start(
      request, profile, token, request.interpreter(), output, error_output,
      request.required_guarantees(), cleanup_outcome::verified,
      "cancelled after start");
  TEST_CHECK(after.start_state() == execution_start_state::started);
  TEST_CHECK(after.termination()->kind() == process_termination_kind::cancelled);
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::cancelled(),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified, execution_failure_kind::cancelled));

  const auto other_request = fixture::request_with_cancellation(
      cancellation_policy::graceful_then_forced(700));
  auto other = cancellation_source::for_request(other_request);
  TEST_CHECK(other.request_cancellation());
  TEST_EXEC_THROWS(error_code::control_mismatch,
      execution_result::cancelled_before_start(
          request, profile, other.token(), {execution_guarantee::cancellation}));
  return EXIT_SUCCESS;
}
