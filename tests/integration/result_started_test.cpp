// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/guarantees.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;

  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  const auto output = stream_capture::retained("stdout\n");
  const auto error_output = stream_capture::retained("stderr\n");

  const auto nonzero = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(2),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified, execution_failure_kind::program_exited_nonzero);
  TEST_CHECK(nonzero.failure() == execution_failure_kind::program_exited_nonzero);
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(0),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified,
          execution_failure_kind::program_exited_nonzero));

  const auto signaled = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::signaled(15),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified,
      execution_failure_kind::program_terminated_by_signal);
  TEST_CHECK(signaled.termination()->kind() == process_termination_kind::signaled);
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(3),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified,
          execution_failure_kind::program_terminated_by_signal));

  const auto limited = execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::resource_limited(resource_limit_kind::address_space),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified,
      execution_failure_kind::resource_limit_exceeded);
  TEST_CHECK(limited.termination()->limit() == resource_limit_kind::address_space);
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_after_start(
          request, profile, request.interpreter(),
          process_termination::resource_limited(resource_limit_kind::file_size),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified,
          execution_failure_kind::resource_limit_exceeded));

  auto without_address = test_support::without_guarantee(
      request.required_guarantees(), execution_guarantee::address_space_limit);
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_after_start(
          request, profile, request.interpreter(),
          process_termination::resource_limited(resource_limit_kind::address_space),
          output, error_output, without_address, cleanup_outcome::verified,
          execution_failure_kind::resource_limit_exceeded));

  auto without_stdout = test_support::without_guarantee(
      request.required_guarantees(), execution_guarantee::complete_stdout_capture);
  const auto log_failure = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(0),
      std::nullopt, error_output, without_stdout, cleanup_outcome::verified,
      execution_failure_kind::log_capture_failed);
  TEST_CHECK(!log_failure.standard_output());
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(0),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified, execution_failure_kind::log_capture_failed));

  auto without_cleanup = test_support::without_guarantee(
      request.required_guarantees(), execution_guarantee::cleanup_verified);
  const auto cleanup_failure = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(0),
      output, error_output, without_cleanup, cleanup_outcome::failed,
      execution_failure_kind::cleanup_failed);
  TEST_CHECK(cleanup_failure.cleanup() == cleanup_outcome::failed);
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(0),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::failed, execution_failure_kind::cleanup_failed));

  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(1),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified, execution_failure_kind::request_rejected));
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(1),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified, static_cast<execution_failure_kind>(255)));
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(1),
          output, error_output, request.required_guarantees(),
          static_cast<cleanup_outcome>(255),
          execution_failure_kind::program_exited_nonzero));
  return EXIT_SUCCESS;
}
