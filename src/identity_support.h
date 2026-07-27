// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pkgexec::detail {

void require_sha256_hex(std::string_view value);
[[nodiscard]] std::string sha256_hex(std::string_view bytes);

class identity_builder final {
public:
  explicit identity_builder(std::string_view domain);
  ~identity_builder();
  identity_builder(const identity_builder&) = delete;
  identity_builder& operator=(const identity_builder&) = delete;
  identity_builder(identity_builder&&) = delete;
  identity_builder& operator=(identity_builder&&) = delete;

  void add_bool(bool value);
  void add_u8(std::uint8_t value);
  void add_u16(std::uint16_t value);
  void add_u32(std::uint32_t value);
  void add_u64(std::uint64_t value);
  void add_i64(std::int64_t value);
  void add_string(std::string_view value);
  void add_optional_u64(const std::optional<std::uint64_t>& value);
  void add_optional_i64(const std::optional<std::int64_t>& value);
  [[nodiscard]] std::string finish();
private:
  void add_bytes(const void* data, std::size_t size);
  struct implementation;
  implementation* implementation_;
  bool finished_ = false;
};

} // namespace pkgexec::detail
