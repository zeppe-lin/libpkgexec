// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "identity_support.h"

#include <libpkgexec/error.h>

#include <openssl/evp.h>

#include <array>
#include <limits>
#include <memory>

namespace pkgexec::detail {
namespace {

[[noreturn]] void fail_crypto(const char* message)
{
  throw error(error_code::identity_failed, message);
}

std::string hex_encode(const unsigned char* bytes, std::size_t size)
{
  static constexpr char alphabet[] = "0123456789abcdef";
  std::string result;
  result.resize(size * 2U);
  for (std::size_t i = 0; i < size; ++i) {
    result[i * 2U] = alphabet[(bytes[i] >> 4U) & 0x0fU];
    result[i * 2U + 1U] = alphabet[bytes[i] & 0x0fU];
  }
  return result;
}

void encode_u64(std::uint64_t value, unsigned char* out)
{
  for (unsigned i = 0; i < 8U; ++i) {
    out[7U - i] = static_cast<unsigned char>(value & 0xffU);
    value >>= 8U;
  }
}

} // namespace

struct identity_builder::implementation final {
  EVP_MD_CTX* context = nullptr;
};

void require_sha256_hex(std::string_view value)
{
  if (value.size() != 64U) {
    throw error(error_code::invalid_identity,
                "SHA-256 identity must contain exactly 64 hexadecimal digits");
  }
  for (const char ch : value) {
    if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) {
      throw error(error_code::invalid_identity,
                  "SHA-256 identity must use lowercase hexadecimal digits");
    }
  }
}

std::string sha256_hex(std::string_view bytes)
{
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned digest_size = 0;
  if (EVP_Digest(bytes.data(), bytes.size(), digest.data(), &digest_size,
                 EVP_sha256(), nullptr) != 1) {
    fail_crypto("cannot calculate SHA-256 digest");
  }
  if (digest_size != 32U) {
    fail_crypto("unexpected SHA-256 digest size");
  }
  return hex_encode(digest.data(), digest_size);
}

identity_builder::identity_builder(std::string_view domain)
    : implementation_(new implementation)
{
  implementation_->context = EVP_MD_CTX_new();
  if (implementation_->context == nullptr) {
    delete implementation_;
    implementation_ = nullptr;
    fail_crypto("cannot allocate identity digest context");
  }
  if (EVP_DigestInit_ex(implementation_->context, EVP_sha256(), nullptr) != 1) {
    EVP_MD_CTX_free(implementation_->context);
    delete implementation_;
    implementation_ = nullptr;
    fail_crypto("cannot initialize identity digest context");
  }
  add_string(domain);
}

identity_builder::~identity_builder()
{
  if (implementation_ != nullptr) {
    EVP_MD_CTX_free(implementation_->context);
    delete implementation_;
  }
}

void identity_builder::add_bytes(const void* data, std::size_t size)
{
  if (finished_) {
    throw error(error_code::identity_failed,
                "identity digest was already finalized");
  }
  if (size != 0U && EVP_DigestUpdate(implementation_->context, data, size) != 1) {
    fail_crypto("cannot update identity digest");
  }
}

void identity_builder::add_bool(bool value)
{
  add_u8(value ? 1U : 0U);
}

void identity_builder::add_u8(std::uint8_t value)
{
  add_bytes(&value, sizeof(value));
}

void identity_builder::add_u16(std::uint16_t value)
{
  std::array<unsigned char, 2> bytes{{
      static_cast<unsigned char>((value >> 8U) & 0xffU),
      static_cast<unsigned char>(value & 0xffU),
  }};
  add_bytes(bytes.data(), bytes.size());
}

void identity_builder::add_u32(std::uint32_t value)
{
  std::array<unsigned char, 4> bytes{};
  for (unsigned i = 0; i < 4U; ++i) {
    bytes[3U - i] = static_cast<unsigned char>(value & 0xffU);
    value >>= 8U;
  }
  add_bytes(bytes.data(), bytes.size());
}

void identity_builder::add_u64(std::uint64_t value)
{
  std::array<unsigned char, 8> bytes{};
  encode_u64(value, bytes.data());
  add_bytes(bytes.data(), bytes.size());
}

void identity_builder::add_i64(std::int64_t value)
{
  add_u64(static_cast<std::uint64_t>(value));
}

void identity_builder::add_string(std::string_view value)
{
  if (value.size() > std::numeric_limits<std::uint64_t>::max()) {
    throw error(error_code::identity_failed, "identity value is too large");
  }
  add_u64(static_cast<std::uint64_t>(value.size()));
  add_bytes(value.data(), value.size());
}

void identity_builder::add_optional_u64(
    const std::optional<std::uint64_t>& value)
{
  add_bool(value.has_value());
  if (value) {
    add_u64(*value);
  }
}

void identity_builder::add_optional_i64(
    const std::optional<std::int64_t>& value)
{
  add_bool(value.has_value());
  if (value) {
    add_i64(*value);
  }
}

std::string identity_builder::finish()
{
  if (finished_) {
    throw error(error_code::identity_failed,
                "identity digest was already finalized");
  }
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  unsigned digest_size = 0;
  if (EVP_DigestFinal_ex(implementation_->context, digest.data(),
                         &digest_size) != 1) {
    fail_crypto("cannot finalize identity digest");
  }
  finished_ = true;
  if (digest_size != 32U) {
    fail_crypto("unexpected identity digest size");
  }
  return hex_encode(digest.data(), digest_size);
}

} // namespace pkgexec::detail
