// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace {
void refresh_checksum(pkgexec::backend_capability_profile_encoding& encoding)
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
  {
    encoding[payload_size + index] = static_cast<std::uint8_t>(
        (digit(hex[index * 2U]) << 4U) | digit(hex[index * 2U + 1U]));
  }
}
}

int main()
{
  using namespace pkgexec;
  const auto profile = fixture::profile(fixture::request());
  const auto encoding = encode_backend_capability_profile(profile);

  auto truncated = encoding;
  truncated.pop_back();
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_backend_capability_profile(truncated));

  auto corrupted = encoding;
  corrupted[corrupted.size() / 2U] ^= 0x01U;
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_backend_capability_profile(corrupted));

  auto bad_magic = encoding;
  bad_magic.front() ^= 0x01U;
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_backend_capability_profile(bad_magic));

  auto bad_version = encoding;
  bad_version[9] ^= 0x01U;
  refresh_checksum(bad_version);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_backend_capability_profile(bad_version));

  auto false_identity = encoding;
  constexpr std::size_t profile_identity_offset = 8U + 2U + 32U;
  false_identity[profile_identity_offset] ^= 0x01U;
  refresh_checksum(false_identity);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_backend_capability_profile(false_identity));

  auto noncanonical = encoding;
  constexpr std::size_t guarantee_count_offset = 8U + 2U + 32U + 32U;
  const auto guarantee_count =
      static_cast<std::size_t>((noncanonical[guarantee_count_offset] << 8U) |
                               noncanonical[guarantee_count_offset + 1U]);
  if (guarantee_count >= 2U)
  {
    const auto first = guarantee_count_offset + 2U;
    std::swap(noncanonical[first], noncanonical[first + 1U]);
    refresh_checksum(noncanonical);
    TEST_EXEC_THROWS(error_code::corrupt_encoding,
                     decode_backend_capability_profile(noncanonical));
  }

  auto oversized = backend_capability_profile_encoding(
      maximum_backend_capability_profile_encoding_size + 1U, 0U);
  TEST_EXEC_THROWS(error_code::corrupt_encoding,
                   decode_backend_capability_profile(oversized));
  return EXIT_SUCCESS;
}
