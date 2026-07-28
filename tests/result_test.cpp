// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

int main()
{
  using namespace pkgexec;

  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  TEST_CHECK(profile.supports(request));

  const auto output = stream_capture::retained("stdout\n");
  const auto error_output = stream_capture::retained("stderr\n");
  const auto success = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      request.required_guarantees(), "first diagnostic");
  const auto same_semantics = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      request.required_guarantees(), "different diagnostic");
  TEST_CHECK(success.status() == execution_status::succeeded);
  TEST_CHECK(success.start_state() == execution_start_state::started);
  TEST_CHECK(success.termination()->kind() == process_termination_kind::exited);
  TEST_CHECK(*success.termination()->value() == 0);
  TEST_CHECK(success.cleanup() == cleanup_outcome::verified);
  TEST_CHECK(!success.failure());
  TEST_CHECK(success.identity() == same_semantics.identity());
  TEST_CHECK(success == same_semantics);

  auto weak_guarantees = request.required_guarantees();
  weak_guarantees.erase(
      std::remove(weak_guarantees.begin(), weak_guarantees.end(),
                  execution_guarantee::network_denied),
      weak_guarantees.end());
  const auto weak_profile = backend_capability_profile::seal(
      fixture::backend("weak"), weak_guarantees);
  TEST_CHECK(!weak_profile.supports(request));
  TEST_EXEC_THROWS(error_code::unsupported_request,
      execution_result::succeeded(
          request, weak_profile, request.interpreter(), output, error_output,
          weak_guarantees));

  const auto rejected = execution_result::failed_before_start(
      request, weak_profile, execution_failure_kind::backend_unsupported,
      {}, "unsupported");
  TEST_CHECK(rejected.status() == execution_status::failed);
  TEST_CHECK(rejected.start_state() == execution_start_state::not_started);
  TEST_CHECK(!rejected.termination());
  TEST_CHECK(rejected.failure() == execution_failure_kind::backend_unsupported);
  TEST_EXEC_THROWS(error_code::unsupported_request,
      execution_result::failed_before_start(
          request, weak_profile,
          execution_failure_kind::resource_admission_failed));
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::program_exited_nonzero));
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::process_start_failed,
          {execution_guarantee::cleanup_verified}));

  const auto nonzero = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(2),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified,
      execution_failure_kind::program_exited_nonzero);
  TEST_CHECK(nonzero.failure() == execution_failure_kind::program_exited_nonzero);
  TEST_CHECK(*nonzero.termination()->value() == 2);

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


  auto cancellation = cancellation_source::for_request(request);
  const auto token = cancellation.token();
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::cancelled_before_start(
          request, profile, token, {execution_guarantee::cancellation}));
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::cancelled));

  TEST_CHECK(cancellation.request_cancellation());
  const auto cancelled_before = execution_result::cancelled_before_start(
      request, profile, token, {execution_guarantee::cancellation},
      "first cancellation diagnostic");
  const auto same_cancelled_before = execution_result::cancelled_before_start(
      request, profile, token, {execution_guarantee::cancellation},
      "different cancellation diagnostic");
  TEST_CHECK(cancelled_before.status() == execution_status::failed);
  TEST_CHECK(cancelled_before.start_state() == execution_start_state::not_started);
  TEST_CHECK(cancelled_before.failure() == execution_failure_kind::cancelled);
  TEST_CHECK(!cancelled_before.termination());
  TEST_CHECK(cancelled_before.identity() == same_cancelled_before.identity());
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::cancelled_before_start(
          request, profile, token, {}));

  const auto other_request = fixture::request_with_cancellation(
      cancellation_policy::graceful_then_forced(700));
  auto other_cancellation = cancellation_source::for_request(other_request);
  TEST_CHECK(other_cancellation.request_cancellation());
  TEST_EXEC_THROWS(error_code::control_mismatch,
      execution_result::cancelled_before_start(
          request, profile, other_cancellation.token(),
          {execution_guarantee::cancellation}));

  const auto cancelled_after = execution_result::cancelled_after_start(
      request, profile, token, request.interpreter(), output, error_output,
      request.required_guarantees(), cleanup_outcome::verified,
      "cancelled after start");
  TEST_CHECK(cancelled_after.start_state() == execution_start_state::started);
  TEST_CHECK(cancelled_after.failure() == execution_failure_kind::cancelled);
  TEST_CHECK(cancelled_after.termination()->kind() ==
             process_termination_kind::cancelled);
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::failed_after_start(
          request, profile, request.interpreter(),
          process_termination::cancelled(), output, error_output,
          request.required_guarantees(), cleanup_outcome::verified,
          execution_failure_kind::cancelled));

  auto pending_cancellation = cancellation_source::for_request(request);
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::cancelled_after_start(
          request, profile, pending_cancellation.token(), request.interpreter(),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::verified));

  auto without_cleanup = request.required_guarantees();
  without_cleanup.erase(
      std::remove(without_cleanup.begin(), without_cleanup.end(),
                  execution_guarantee::cleanup_verified),
      without_cleanup.end());
  const auto cleanup_failed = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(0),
      output, error_output, without_cleanup,
      cleanup_outcome::failed, execution_failure_kind::cleanup_failed);
  TEST_CHECK(cleanup_failed.cleanup() == cleanup_outcome::failed);
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_after_start(
          request, profile, request.interpreter(), process_termination::exited(0),
          output, error_output, request.required_guarantees(),
          cleanup_outcome::failed, execution_failure_kind::cleanup_failed));

  auto without_stdout = request.required_guarantees();
  without_stdout.erase(
      std::remove(without_stdout.begin(), without_stdout.end(),
                  execution_guarantee::complete_stdout_capture),
      without_stdout.end());
  const auto log_failure = execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(0),
      std::nullopt, error_output, without_stdout, cleanup_outcome::verified,
      execution_failure_kind::log_capture_failed);
  TEST_CHECK(!log_failure.standard_output());

  return EXIT_SUCCESS;
}
