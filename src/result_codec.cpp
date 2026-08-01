// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgexec/result_codec.h>

#include <libpkgexec/error.h>

#include "identity_support.h"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pkgexec {
namespace {

constexpr std::array<std::uint8_t, 8> encoding_magic{
    'P', 'K', 'G', 'E', 'X', 'R', '1', 0};
constexpr std::size_t checksum_size = 32U;
constexpr std::size_t maximum_diagnostic_size = 1024U * 1024U;
constexpr std::size_t maximum_capture_material_size =
    maximum_execution_result_encoding_size - checksum_size;
constexpr std::size_t maximum_guarantee_count = 32U;

[[noreturn]] void corrupt(const std::string& message)
{
  throw error(error_code::corrupt_encoding, message);
}

[[noreturn]] void authority_mismatch(const std::string& message)
{
  throw error(error_code::authority_mismatch, message);
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

  void u32(std::uint32_t value)
  {
    for (int shift = 24; shift >= 0; shift -= 8)
      byte(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }

  void u64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
      byte(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }

  void boolean(bool value)
  {
    byte(value ? 1U : 0U);
  }

  void raw(const std::uint8_t* data, std::size_t size)
  {
    if (size == 0U)
      return;
    if (size > maximum_execution_result_encoding_size - output_.size())
      throw error(error_code::inconsistent_result,
                  "execution-result encoding exceeds maximum size");
    output_.insert(output_.end(), data, data + size);
  }

  void text(std::string_view value)
  {
    if (value.size() > std::numeric_limits<std::uint32_t>::max())
      throw error(error_code::inconsistent_result,
                  "execution-result text field is too large");
    u32(static_cast<std::uint32_t>(value.size()));
    raw(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
  }

  void identity(std::string_view value)
  {
    detail::require_sha256_hex(value);
    for (std::size_t index = 0; index < value.size(); index += 2U)
    {
      const auto high = digit(value[index]);
      const auto low = digit(value[index + 1U]);
      byte(static_cast<std::uint8_t>((high << 4U) | low));
    }
  }

  void capture(const std::optional<stream_capture>& value)
  {
    boolean(value.has_value());
    if (!value)
      return;
    u64(value->byte_count());
    identity(value->digest().hex());
    boolean(value->material().has_value());
    if (value->material())
      text(*value->material());
  }

  const execution_result_encoding& output() const noexcept
  {
    return output_;
  }

  execution_result_encoding finish()
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
    if (output_.size() > maximum_execution_result_encoding_size)
      throw error(error_code::inconsistent_result,
                  "execution-result encoding exceeds maximum size");
  }

  execution_result_encoding output_;
};

class reader final {
public:
  reader(const execution_result_encoding& input, std::size_t limit)
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

  std::uint32_t u32()
  {
    std::uint32_t value = 0U;
    for (int index = 0; index < 4; ++index)
      value = (value << 8U) | byte();
    return value;
  }

  std::uint64_t u64()
  {
    std::uint64_t value = 0U;
    for (int index = 0; index < 8; ++index)
      value = (value << 8U) | byte();
    return value;
  }

  bool boolean()
  {
    const auto value = byte();
    if (value > 1U)
      corrupt("execution-result encoding contains an invalid boolean");
    return value == 1U;
  }

  std::string text(std::size_t maximum)
  {
    const auto size = static_cast<std::size_t>(u32());
    if (size > maximum)
      corrupt("execution-result text field exceeds its limit");
    require(size);
    std::string value(
        reinterpret_cast<const char*>(input_.data() + offset_), size);
    offset_ += size;
    return value;
  }

  std::string identity()
  {
    static constexpr char digits[] = "0123456789abcdef";
    require(32U);
    std::string value(64U, '0');
    for (std::size_t index = 0; index < 32U; ++index)
    {
      const auto current = input_[offset_++];
      value[index * 2U] = digits[(current >> 4U) & 0x0fU];
      value[index * 2U + 1U] = digits[current & 0x0fU];
    }
    return value;
  }

  std::optional<stream_capture> capture()
  {
    if (!boolean())
      return std::nullopt;
    const auto count = u64();
    auto digest = sha256_digest(identity());
    if (!boolean())
      return stream_capture::observed(count, std::move(digest));
    auto material = text(maximum_capture_material_size);
    auto retained = stream_capture::retained(std::move(material));
    if (retained.byte_count() != count || retained.digest() != digest)
      corrupt("execution-result retained stream evidence is inconsistent");
    return retained;
  }

  void finish() const
  {
    if (offset_ != limit_)
      corrupt("execution-result encoding contains trailing payload bytes");
  }

private:
  void require(std::size_t size) const
  {
    if (size > limit_ - offset_)
      corrupt("execution-result encoding is truncated");
  }

  const execution_result_encoding& input_;
  std::size_t limit_;
  std::size_t offset_ = 0U;
};

execution_status decode_status(std::uint8_t value)
{
  switch (value)
  {
    case 0U: return execution_status::succeeded;
    case 1U: return execution_status::failed;
  }
  corrupt("execution-result encoding contains an unknown status");
}

execution_start_state decode_start_state(std::uint8_t value)
{
  switch (value)
  {
    case 0U: return execution_start_state::not_started;
    case 1U: return execution_start_state::started;
  }
  corrupt("execution-result encoding contains an unknown start state");
}

execution_guarantee decode_guarantee(std::uint8_t value)
{
  if (value > static_cast<std::uint8_t>(execution_guarantee::process_count_limit))
    corrupt("execution-result encoding contains an unknown guarantee");
  return static_cast<execution_guarantee>(value);
}

cleanup_outcome decode_cleanup(std::uint8_t value)
{
  switch (value)
  {
    case 0U: return cleanup_outcome::not_required;
    case 1U: return cleanup_outcome::verified;
    case 2U: return cleanup_outcome::failed;
  }
  corrupt("execution-result encoding contains an unknown cleanup outcome");
}

execution_failure_kind decode_failure(std::uint8_t value)
{
  if (value > static_cast<std::uint8_t>(execution_failure_kind::cleanup_failed))
    corrupt("execution-result encoding contains an unknown failure kind");
  return static_cast<execution_failure_kind>(value);
}

resource_limit_kind decode_limit(std::uint8_t value)
{
  if (value > static_cast<std::uint8_t>(resource_limit_kind::process_count))
    corrupt("execution-result encoding contains an unknown resource limit");
  return static_cast<resource_limit_kind>(value);
}

process_termination decode_termination(reader& input)
{
  const auto kind = input.byte();
  const bool has_value = input.boolean();
  const auto value = has_value ? std::optional<std::uint32_t>(input.u32())
                               : std::nullopt;
  const bool has_limit = input.boolean();
  const auto limit = has_limit
      ? std::optional<resource_limit_kind>(decode_limit(input.byte()))
      : std::nullopt;

  switch (kind)
  {
    case 0U:
      if (!value || limit)
        corrupt("execution-result exit termination has invalid fields");
      return process_termination::exited(*value);
    case 1U:
      if (!value || limit)
        corrupt("execution-result signal termination has invalid fields");
      return process_termination::signaled(*value);
    case 2U:
      if (value || limit)
        corrupt("execution-result cancellation termination has invalid fields");
      return process_termination::cancelled();
    case 3U:
      if (value || !limit)
        corrupt("execution-result limit termination has invalid fields");
      return process_termination::resource_limited(*limit);
  }
  corrupt("execution-result encoding contains an unknown termination kind");
}

void encode_termination(writer& output,
                        const std::optional<process_termination>& value)
{
  output.boolean(value.has_value());
  if (!value)
    return;
  output.byte(static_cast<std::uint8_t>(value->kind()));
  output.boolean(value->value().has_value());
  if (value->value())
    output.u32(*value->value());
  output.boolean(value->limit().has_value());
  if (value->limit())
    output.byte(static_cast<std::uint8_t>(*value->limit()));
}

execution_result rebuild_result(
    execution_status status,
    execution_start_state start_state,
    execution_request request,
    backend_capability_profile backend,
    std::optional<interpreter_identity> observed_interpreter,
    std::optional<process_termination> termination,
    std::optional<stream_capture> standard_output,
    std::optional<stream_capture> standard_error,
    std::vector<execution_guarantee> established_guarantees,
    cleanup_outcome cleanup,
    std::optional<execution_failure_kind> failure,
    std::string diagnostic)
{
  if (status == execution_status::succeeded)
  {
    if (start_state != execution_start_state::started ||
        !observed_interpreter || !termination || failure ||
        cleanup != cleanup_outcome::verified)
      corrupt("execution-result success has an invalid evidence shape");
    return execution_result::succeeded(
        std::move(request), std::move(backend),
        std::move(*observed_interpreter), std::move(standard_output),
        std::move(standard_error), std::move(established_guarantees),
        std::move(diagnostic));
  }

  if (!failure)
    corrupt("execution-result failure lacks a failure kind");

  if (start_state == execution_start_state::not_started)
  {
    if (observed_interpreter || termination || standard_output ||
        standard_error || cleanup != cleanup_outcome::not_required)
      corrupt("not-started execution result has started evidence");
    if (*failure == execution_failure_kind::cancelled)
    {
      auto cancellation = cancellation_source::for_request(request);
      if (!cancellation.request_cancellation())
        corrupt("cannot reconstruct execution cancellation evidence");
      return execution_result::cancelled_before_start(
          std::move(request), std::move(backend), cancellation.token(),
          std::move(established_guarantees), std::move(diagnostic));
    }
    return execution_result::failed_before_start(
        std::move(request), std::move(backend), *failure,
        std::move(established_guarantees), std::move(diagnostic));
  }

  if (!observed_interpreter || !termination)
    corrupt("started execution result lacks process evidence");
  if (*failure == execution_failure_kind::cancelled)
  {
    auto cancellation = cancellation_source::for_request(request);
    if (!cancellation.request_cancellation())
      corrupt("cannot reconstruct execution cancellation evidence");
    return execution_result::cancelled_after_start(
        std::move(request), std::move(backend), cancellation.token(),
        std::move(*observed_interpreter), std::move(standard_output),
        std::move(standard_error), std::move(established_guarantees), cleanup,
        std::move(diagnostic));
  }
  return execution_result::failed_after_start(
      std::move(request), std::move(backend),
      std::move(*observed_interpreter), std::move(*termination),
      std::move(standard_output), std::move(standard_error),
      std::move(established_guarantees), cleanup, *failure,
      std::move(diagnostic));
}

} // namespace

execution_result_encoding encode_execution_result(const execution_result& result)
{
  writer output;
  output.raw(encoding_magic.data(), encoding_magic.size());
  output.u16(execution_result_encoding_version);
  output.identity(result.request().identity().hex());
  output.identity(result.backend().identity().hex());
  output.identity(result.identity().hex());
  output.byte(static_cast<std::uint8_t>(result.status()));
  output.byte(static_cast<std::uint8_t>(result.start_state()));
  output.boolean(result.observed_interpreter().has_value());
  if (result.observed_interpreter())
    output.identity(result.observed_interpreter()->hex());
  encode_termination(output, result.termination());
  output.capture(result.standard_output());
  output.capture(result.standard_error());
  if (result.established_guarantees().size() > maximum_guarantee_count)
    throw error(error_code::inconsistent_result,
                "execution-result guarantee count exceeds its limit");
  output.u16(static_cast<std::uint16_t>(
      result.established_guarantees().size()));
  for (const auto guarantee : result.established_guarantees())
    output.byte(static_cast<std::uint8_t>(guarantee));
  output.byte(static_cast<std::uint8_t>(result.cleanup()));
  output.boolean(result.failure().has_value());
  if (result.failure())
    output.byte(static_cast<std::uint8_t>(*result.failure()));
  if (result.diagnostic().size() > maximum_diagnostic_size)
    throw error(error_code::inconsistent_result,
                "execution-result diagnostic exceeds its limit");
  output.text(result.diagnostic());

  const auto& payload = output.output();
  const auto checksum = detail::sha256_hex(std::string_view(
      reinterpret_cast<const char*>(payload.data()), payload.size()));
  output.identity(checksum);
  return output.finish();
}

execution_result decode_execution_result(
    const execution_result_encoding& encoding,
    execution_request request,
    backend_capability_profile backend)
{
  try
  {
    if (encoding.size() > maximum_execution_result_encoding_size)
      corrupt("execution-result encoding exceeds maximum size");
    if (encoding.size() < encoding_magic.size() + 2U + checksum_size)
      corrupt("execution-result encoding is truncated");

    const auto payload_size = encoding.size() - checksum_size;
    const auto actual_checksum = detail::sha256_hex(std::string_view(
        reinterpret_cast<const char*>(encoding.data()), payload_size));
    std::string retained_checksum(64U, '0');
    static constexpr char checksum_digits[] = "0123456789abcdef";
    for (std::size_t index = 0; index < checksum_size; ++index)
    {
      const auto current = encoding[payload_size + index];
      retained_checksum[index * 2U] =
          checksum_digits[(current >> 4U) & 0x0fU];
      retained_checksum[index * 2U + 1U] =
          checksum_digits[current & 0x0fU];
    }
    if (retained_checksum != actual_checksum)
      corrupt("execution-result encoding checksum mismatch");

    reader input(encoding, payload_size);

    for (const auto expected : encoding_magic)
      if (input.byte() != expected)
        corrupt("execution-result encoding has invalid magic");
    if (input.u16() != execution_result_encoding_version)
      corrupt("execution-result encoding version is unsupported");

    const auto request_identity = input.identity();
    const auto backend_identity = input.identity();
    const auto evidence_identity = input.identity();
    if (request.identity().hex() != request_identity)
      authority_mismatch(
          "execution-result record belongs to another execution request");
    if (backend.identity().hex() != backend_identity)
      authority_mismatch(
          "execution-result record belongs to another backend profile");

    const auto status = decode_status(input.byte());
    const auto start_state = decode_start_state(input.byte());
    std::optional<interpreter_identity> observed_interpreter;
    if (input.boolean())
      observed_interpreter = interpreter_identity::from_sha256(input.identity());
    std::optional<process_termination> termination;
    if (input.boolean())
      termination = decode_termination(input);
    auto standard_output = input.capture();
    auto standard_error = input.capture();
    const auto guarantee_count = static_cast<std::size_t>(input.u16());
    if (guarantee_count > maximum_guarantee_count)
      corrupt("execution-result guarantee count exceeds its limit");
    std::vector<execution_guarantee> guarantees;
    guarantees.reserve(guarantee_count);
    for (std::size_t index = 0; index < guarantee_count; ++index)
      guarantees.push_back(decode_guarantee(input.byte()));
    if (!std::is_sorted(guarantees.begin(), guarantees.end()) ||
        std::adjacent_find(guarantees.begin(), guarantees.end()) !=
            guarantees.end())
      corrupt("execution-result guarantees are not canonical");
    const auto cleanup = decode_cleanup(input.byte());
    std::optional<execution_failure_kind> failure;
    if (input.boolean())
      failure = decode_failure(input.byte());
    auto diagnostic = input.text(maximum_diagnostic_size);
    input.finish();

    execution_result decoded = rebuild_result(
        status, start_state, std::move(request), std::move(backend),
        std::move(observed_interpreter), std::move(termination),
        std::move(standard_output), std::move(standard_error),
        std::move(guarantees), cleanup, failure, std::move(diagnostic));
    if (decoded.identity().hex() != evidence_identity)
      corrupt("execution-result evidence identity mismatch");

    const auto canonical = encode_execution_result(decoded);
    if (canonical != encoding)
      corrupt("execution-result encoding is not canonical");
    return decoded;
  }
  catch (const error& failure)
  {
    if (failure.code() == error_code::corrupt_encoding ||
        failure.code() == error_code::authority_mismatch ||
        failure.code() == error_code::identity_failed)
      throw;
    throw error(
        error_code::corrupt_encoding,
        std::string("execution-result encoding violates the result contract: ") +
            failure.what());
  }
}

} // namespace pkgexec
