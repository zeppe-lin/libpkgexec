// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgexec/profile_codec.h>

#include <libpkgexec/error.h>

#include "identity_support.h"

#include <array>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pkgexec {
namespace {

constexpr std::array<std::uint8_t, 8> encoding_magic{
    'P', 'K', 'G', 'E', 'X', 'P', '1', 0};
constexpr std::size_t checksum_size = 32U;
constexpr std::size_t maximum_guarantee_count = 32U;

[[noreturn]] void corrupt(const std::string& message)
{
  throw error(error_code::corrupt_encoding, message);
}

class writer final {
public:
  void byte(std::uint8_t value)
  {
    output_.push_back(value);
    check_size();
  }

  void u16(std::uint16_t value)
  {
    byte(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    byte(static_cast<std::uint8_t>(value & 0xffU));
  }

  void raw(const std::uint8_t* data, std::size_t size)
  {
    if (size == 0U)
      return;
    if (size > maximum_backend_capability_profile_encoding_size - output_.size())
    {
      throw error(
          error_code::invalid_capability_profile,
          "backend-capability-profile encoding exceeds maximum size");
    }
    output_.insert(output_.end(), data, data + size);
  }

  void identity(std::string_view value)
  {
    detail::require_sha256_hex(value);
    for (std::size_t index = 0U; index < value.size(); index += 2U)
    {
      const auto high = digit(value[index]);
      const auto low = digit(value[index + 1U]);
      byte(static_cast<std::uint8_t>((high << 4U) | low));
    }
  }

  const backend_capability_profile_encoding& output() const noexcept
  {
    return output_;
  }

  backend_capability_profile_encoding finish()
  {
    return std::move(output_);
  }

private:
  static std::uint8_t digit(char value)
  {
    if (value >= '0' && value <= '9')
      return static_cast<std::uint8_t>(value - '0');
    return static_cast<std::uint8_t>(value - 'a' + 10);
  }

  void check_size() const
  {
    if (output_.size() > maximum_backend_capability_profile_encoding_size)
    {
      throw error(
          error_code::invalid_capability_profile,
          "backend-capability-profile encoding exceeds maximum size");
    }
  }

  backend_capability_profile_encoding output_;
};

class reader final {
public:
  reader(
      const backend_capability_profile_encoding& input,
      std::size_t limit)
      : input_(input), limit_(limit)
  {
  }

  std::uint8_t byte()
  {
    require(1U);
    return input_[offset_++];
  }

  std::uint16_t u16()
  {
    std::uint16_t value = 0U;
    for (int index = 0; index < 2; ++index)
      value = static_cast<std::uint16_t>((value << 8U) | byte());
    return value;
  }

  std::string identity()
  {
    static constexpr char digits[] = "0123456789abcdef";
    require(32U);
    std::string value(64U, '0');
    for (std::size_t index = 0U; index < 32U; ++index)
    {
      const auto current = input_[offset_++];
      value[index * 2U] = digits[(current >> 4U) & 0x0fU];
      value[index * 2U + 1U] = digits[current & 0x0fU];
    }
    return value;
  }

  void finish() const
  {
    if (offset_ != limit_)
      corrupt("backend-capability-profile encoding contains trailing payload bytes");
  }

private:
  void require(std::size_t size) const
  {
    if (offset_ > limit_ || size > limit_ - offset_)
      corrupt("backend-capability-profile encoding is truncated");
  }

  const backend_capability_profile_encoding& input_;
  std::size_t limit_;
  std::size_t offset_ = 0U;
};

execution_guarantee decode_guarantee(std::uint8_t value)
{
  if (value > static_cast<std::uint8_t>(execution_guarantee::process_count_limit))
  {
    corrupt(
        "backend-capability-profile encoding contains an unknown guarantee");
  }
  return static_cast<execution_guarantee>(value);
}

} // namespace

backend_capability_profile_encoding encode_backend_capability_profile(
    const backend_capability_profile& profile)
{
  writer output;
  output.raw(encoding_magic.data(), encoding_magic.size());
  output.u16(backend_capability_profile_encoding_version);
  output.identity(profile.backend().hex());
  output.identity(profile.identity().hex());
  if (profile.guarantees().size() > maximum_guarantee_count)
  {
    throw error(
        error_code::invalid_capability_profile,
        "backend-capability-profile guarantee count exceeds its limit");
  }
  output.u16(static_cast<std::uint16_t>(profile.guarantees().size()));
  for (const auto guarantee : profile.guarantees())
    output.byte(static_cast<std::uint8_t>(guarantee));

  const auto& payload = output.output();
  const auto checksum = detail::sha256_hex(std::string_view(
      reinterpret_cast<const char*>(payload.data()), payload.size()));
  output.identity(checksum);
  return output.finish();
}

backend_capability_profile decode_backend_capability_profile(
    const backend_capability_profile_encoding& encoding)
{
  try
  {
    if (encoding.size() > maximum_backend_capability_profile_encoding_size)
      corrupt("backend-capability-profile encoding exceeds maximum size");
    if (encoding.size() < encoding_magic.size() + 2U + checksum_size)
      corrupt("backend-capability-profile encoding is truncated");

    const auto payload_size = encoding.size() - checksum_size;
    const auto actual_checksum = detail::sha256_hex(std::string_view(
        reinterpret_cast<const char*>(encoding.data()), payload_size));
    std::string retained_checksum(64U, '0');
    static constexpr char checksum_digits[] = "0123456789abcdef";
    for (std::size_t index = 0U; index < checksum_size; ++index)
    {
      const auto current = encoding[payload_size + index];
      retained_checksum[index * 2U] =
          checksum_digits[(current >> 4U) & 0x0fU];
      retained_checksum[index * 2U + 1U] =
          checksum_digits[current & 0x0fU];
    }
    if (retained_checksum != actual_checksum)
      corrupt("backend-capability-profile encoding checksum mismatch");

    reader input(encoding, payload_size);
    for (const auto expected : encoding_magic)
    {
      if (input.byte() != expected)
        corrupt("backend-capability-profile encoding has invalid magic");
    }
    if (input.u16() != backend_capability_profile_encoding_version)
      corrupt("backend-capability-profile encoding version is unsupported");

    auto backend = backend_identity::from_sha256(input.identity());
    const auto retained_identity = input.identity();
    const auto guarantee_count = static_cast<std::size_t>(input.u16());
    if (guarantee_count > maximum_guarantee_count)
      corrupt("backend-capability-profile guarantee count exceeds its limit");
    std::vector<execution_guarantee> guarantees;
    guarantees.reserve(guarantee_count);
    for (std::size_t index = 0U; index < guarantee_count; ++index)
      guarantees.push_back(decode_guarantee(input.byte()));
    input.finish();

    auto decoded = backend_capability_profile::seal(
        std::move(backend), std::move(guarantees));
    if (decoded.identity().hex() != retained_identity)
      corrupt("backend-capability-profile identity mismatch");
    if (encode_backend_capability_profile(decoded) != encoding)
      corrupt("backend-capability-profile encoding is not canonical");
    return decoded;
  }
  catch (const error& failure)
  {
    if (failure.code() == error_code::corrupt_encoding ||
        failure.code() == error_code::identity_failed)
    {
      throw;
    }
    throw error(
        error_code::corrupt_encoding,
        std::string(
            "backend-capability-profile encoding violates the profile contract: ") +
            failure.what());
  }
}

} // namespace pkgexec
