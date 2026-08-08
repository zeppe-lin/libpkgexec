// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/guarantees.h"
#include "../support/result.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

namespace {
int round_trip(const pkgexec::execution_result& value)
{
  const auto first = pkgexec::encode_execution_result(value);
  const auto decoded = pkgexec::decode_execution_result(
      first, value.request(), value.backend());
  if (!test_support::exact_result(value, decoded)) {
    return EXIT_FAILURE;
  }
  return pkgexec::encode_execution_result(decoded) == first
      ? EXIT_SUCCESS : EXIT_FAILURE;
}
}

int main()
{
  using namespace pkgexec;
  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  const auto output = stream_capture::retained("stdout\n");
  const auto error_output = stream_capture::retained("stderr\n");

  const auto success = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      request.required_guarantees(), "success diagnostic\nwith detail");
  TEST_CHECK(round_trip(success) == EXIT_SUCCESS);
  TEST_CHECK(encode_execution_result(success).size() <
             maximum_execution_result_encoding_size);

  const auto observed_success = execution_result::succeeded(
      request, profile, request.interpreter(),
      stream_capture::observed(7U, sha256_digest::of_bytes("stdout")),
      stream_capture::observed(7U, sha256_digest::of_bytes("stderr")),
      request.required_guarantees(), "digest-only streams");
  TEST_CHECK(round_trip(observed_success) == EXIT_SUCCESS);

  auto weak = request.required_guarantees();
  weak.erase(std::remove(weak.begin(), weak.end(),
                         execution_guarantee::network_denied), weak.end());
  const auto weak_profile = backend_capability_profile::seal(
      fixture::backend("codec-weak"), weak);
  TEST_CHECK(round_trip(execution_result::failed_before_start(
      request, weak_profile, execution_failure_kind::backend_unsupported,
      {}, "unsupported backend")) == EXIT_SUCCESS);

  auto cancellation = cancellation_source::for_request(request);
  TEST_CHECK(cancellation.request_cancellation());
  TEST_CHECK(round_trip(execution_result::cancelled_before_start(
      request, profile, cancellation.token(),
      {execution_guarantee::cancellation}, "cancelled before start")) ==
      EXIT_SUCCESS);

  TEST_CHECK(round_trip(execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(23U),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified, execution_failure_kind::program_exited_nonzero,
      "nonzero exit")) == EXIT_SUCCESS);
  TEST_CHECK(round_trip(execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::signaled(15U),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified,
      execution_failure_kind::program_terminated_by_signal, "signal")) ==
      EXIT_SUCCESS);
  TEST_CHECK(round_trip(execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::resource_limited(resource_limit_kind::address_space),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified, execution_failure_kind::resource_limit_exceeded,
      "limit")) == EXIT_SUCCESS);
  TEST_CHECK(round_trip(execution_result::cancelled_after_start(
      request, profile, cancellation.token(), request.interpreter(), output,
      error_output, request.required_guarantees(), cleanup_outcome::verified,
      "cancelled after start")) == EXIT_SUCCESS);

  const auto without_stdout = test_support::without_guarantee(
      request.required_guarantees(), execution_guarantee::complete_stdout_capture);
  TEST_CHECK(round_trip(execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(0U),
      std::nullopt, error_output, without_stdout, cleanup_outcome::verified,
      execution_failure_kind::log_capture_failed, "capture failed")) ==
      EXIT_SUCCESS);

  const auto without_cleanup = test_support::without_guarantee(
      request.required_guarantees(), execution_guarantee::cleanup_verified);
  TEST_CHECK(round_trip(execution_result::failed_after_start(
      request, profile, request.interpreter(), process_termination::exited(0U),
      output, error_output, without_cleanup, cleanup_outcome::failed,
      execution_failure_kind::cleanup_failed, "cleanup failed")) ==
      EXIT_SUCCESS);
  return EXIT_SUCCESS;
}
