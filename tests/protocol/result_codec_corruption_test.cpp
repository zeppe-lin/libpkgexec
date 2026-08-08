// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <cstdint>
#include <string_view>

namespace {
void refresh_checksum(pkgexec::execution_result_encoding& encoding)
{
  const auto payload_size = encoding.size() - 32U;
  const auto hex = pkgexec::sha256_digest::of_bytes(std::string_view(
      reinterpret_cast<const char*>(encoding.data()), payload_size)).hex();
  const auto digit = [](char value) -> std::uint8_t {
    if (value >= '0' && value <= '9') {
      return static_cast<std::uint8_t>(value - '0');
    }
    return static_cast<std::uint8_t>(value - 'a' + 10);
  };
  for (std::size_t index = 0; index < 32U; ++index) {
    encoding[payload_size + index] = static_cast<std::uint8_t>(
        (digit(hex[index * 2U]) << 4U) | digit(hex[index * 2U + 1U]));
  }
}
}

int main()
{
  using namespace pkgexec;
  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  const auto success = execution_result::succeeded(
      request, profile, request.interpreter(), stream_capture::retained("out"),
      stream_capture::retained("err"), request.required_guarantees(), "diag");
  const auto encoding = encode_execution_result(success);

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

  auto bad_version = encoding;
  bad_version[9] ^= 0x01U;
  refresh_checksum(bad_version);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_execution_result(bad_version, request, profile));

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
