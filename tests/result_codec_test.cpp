// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

bool exact_result(const pkgexec::execution_result& lhs,
                  const pkgexec::execution_result& rhs)
{
  return lhs.status() == rhs.status() &&
      lhs.start_state() == rhs.start_state() &&
      lhs.request().identity() == rhs.request().identity() &&
      lhs.backend().identity() == rhs.backend().identity() &&
      lhs.observed_interpreter() == rhs.observed_interpreter() &&
      lhs.termination() == rhs.termination() &&
      lhs.standard_output() == rhs.standard_output() &&
      lhs.standard_error() == rhs.standard_error() &&
      lhs.established_guarantees() == rhs.established_guarantees() &&
      lhs.cleanup() == rhs.cleanup() &&
      lhs.failure() == rhs.failure() &&
      lhs.diagnostic() == rhs.diagnostic() &&
      lhs.identity() == rhs.identity();
}


void refresh_checksum(pkgexec::execution_result_encoding& encoding)
{
  const auto payload_size = encoding.size() - 32U;
  const auto hex = pkgexec::sha256_digest::of_bytes(std::string_view(
      reinterpret_cast<const char*>(encoding.data()), payload_size)).hex();
  const auto digit = [](char value) -> std::uint8_t {
    if (value >= '0' && value <= '9')
      return static_cast<std::uint8_t>(value - '0');
    return static_cast<std::uint8_t>(value - 'a' + 10);
  };
  for (std::size_t index = 0; index < 32U; ++index)
    encoding[payload_size + index] = static_cast<std::uint8_t>(
        (digit(hex[index * 2U]) << 4U) | digit(hex[index * 2U + 1U]));
}

int round_trip(const pkgexec::execution_result& value)
{
  const auto first = pkgexec::encode_execution_result(value);
  const auto decoded = pkgexec::decode_execution_result(
      first, value.request(), value.backend());
  if (!exact_result(value, decoded))
    return EXIT_FAILURE;
  const auto second = pkgexec::encode_execution_result(decoded);
  if (first != second)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

} // namespace

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

  const auto observed_success = execution_result::succeeded(
      request, profile, request.interpreter(),
      stream_capture::observed(
          7U, sha256_digest::of_bytes("stdout")),
      stream_capture::observed(
          7U, sha256_digest::of_bytes("stderr")),
      request.required_guarantees(), "digest-only streams");
  TEST_CHECK(round_trip(observed_success) == EXIT_SUCCESS);

  auto weak_guarantees = request.required_guarantees();
  weak_guarantees.erase(
      std::remove(weak_guarantees.begin(), weak_guarantees.end(),
                  execution_guarantee::network_denied),
      weak_guarantees.end());
  const auto weak_profile = backend_capability_profile::seal(
      fixture::backend("codec-weak"), weak_guarantees);
  const auto rejected = execution_result::failed_before_start(
      request, weak_profile, execution_failure_kind::backend_unsupported,
      {}, "unsupported backend");
  TEST_CHECK(round_trip(rejected) == EXIT_SUCCESS);

  auto cancellation = cancellation_source::for_request(request);
  TEST_CHECK(cancellation.request_cancellation());
  const auto cancelled_before = execution_result::cancelled_before_start(
      request, profile, cancellation.token(),
      {execution_guarantee::cancellation}, "cancelled before start");
  TEST_CHECK(round_trip(cancelled_before) == EXIT_SUCCESS);

  const auto nonzero = execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::exited(23U), output, error_output,
      request.required_guarantees(), cleanup_outcome::verified,
      execution_failure_kind::program_exited_nonzero, "nonzero exit");
  TEST_CHECK(round_trip(nonzero) == EXIT_SUCCESS);

  const auto signaled = execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::signaled(15U), output, error_output,
      request.required_guarantees(), cleanup_outcome::verified,
      execution_failure_kind::program_terminated_by_signal, "signal");
  TEST_CHECK(round_trip(signaled) == EXIT_SUCCESS);

  const auto limited = execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::resource_limited(
          resource_limit_kind::address_space),
      output, error_output, request.required_guarantees(),
      cleanup_outcome::verified,
      execution_failure_kind::resource_limit_exceeded, "limit");
  TEST_CHECK(round_trip(limited) == EXIT_SUCCESS);

  const auto cancelled_after = execution_result::cancelled_after_start(
      request, profile, cancellation.token(), request.interpreter(), output,
      error_output, request.required_guarantees(), cleanup_outcome::verified,
      "cancelled after start");
  TEST_CHECK(round_trip(cancelled_after) == EXIT_SUCCESS);

  auto without_stdout = request.required_guarantees();
  without_stdout.erase(
      std::remove(without_stdout.begin(), without_stdout.end(),
                  execution_guarantee::complete_stdout_capture),
      without_stdout.end());
  const auto log_failure = execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::exited(0U), std::nullopt, error_output,
      without_stdout, cleanup_outcome::verified,
      execution_failure_kind::log_capture_failed, "capture failed");
  TEST_CHECK(round_trip(log_failure) == EXIT_SUCCESS);

  auto without_cleanup = request.required_guarantees();
  without_cleanup.erase(
      std::remove(without_cleanup.begin(), without_cleanup.end(),
                  execution_guarantee::cleanup_verified),
      without_cleanup.end());
  const auto cleanup_failure = execution_result::failed_after_start(
      request, profile, request.interpreter(),
      process_termination::exited(0U), output, error_output,
      without_cleanup, cleanup_outcome::failed,
      execution_failure_kind::cleanup_failed, "cleanup failed");
  TEST_CHECK(round_trip(cleanup_failure) == EXIT_SUCCESS);

  const auto encoding = encode_execution_result(success);
  TEST_CHECK(encoding.size() < maximum_execution_result_encoding_size);

  auto truncated = encoding;
  truncated.pop_back();
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
      decode_execution_result(truncated, request, profile));

  auto corrupted = encoding;
  corrupted[corrupted.size() / 2U] ^= 0x01U;
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
      decode_execution_result(corrupted, request, profile));

  auto bad_magic = encoding;
  bad_magic.front() ^= 0x01U;
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
      decode_execution_result(bad_magic, request, profile));

  auto invalid_shape = encoding;
  constexpr std::size_t start_state_offset = 8U + 2U + 32U + 32U + 32U + 1U;
  invalid_shape[start_state_offset] = 0U;
  refresh_checksum(invalid_shape);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
      decode_execution_result(invalid_shape, request, profile));

  auto false_identity = encoding;
  constexpr std::size_t evidence_identity_offset = 8U + 2U + 32U + 32U;
  false_identity[evidence_identity_offset] ^= 0x01U;
  refresh_checksum(false_identity);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
      decode_execution_result(false_identity, request, profile));

  auto oversized = execution_result_encoding(
      maximum_execution_result_encoding_size + 1U, 0U);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
      decode_execution_result(oversized, request, profile));

  const auto foreign_request = fixture::request_with_limits(
      resource_limits::make(2000U, 1024U * 1024U, std::nullopt, 128U, 64U),
      cancellation_policy::graceful_then_forced(500U));
  TEST_EXEC_THROWS(error_code::authority_mismatch,
      decode_execution_result(encoding, foreign_request, profile));

  const auto foreign_profile = backend_capability_profile::seal(
      fixture::backend("foreign-codec-backend"),
      request.required_guarantees());
  TEST_EXEC_THROWS(error_code::authority_mismatch,
      decode_execution_result(encoding, request, foreign_profile));

  return EXIT_SUCCESS;
}
