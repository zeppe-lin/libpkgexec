// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <libpkgexec/export.h>
#include <libpkgexec/result.h>

namespace pkgexec {

inline constexpr std::uint16_t backend_capability_profile_encoding_version = 1;
inline constexpr std::size_t maximum_backend_capability_profile_encoding_size =
    4096U;

using backend_capability_profile_encoding = std::vector<std::uint8_t>;

/*! \brief Encode one exact backend capability profile as durable owner evidence. */
[[nodiscard]] PKGEXEC_API backend_capability_profile_encoding
encode_backend_capability_profile(const backend_capability_profile& profile);

/*! \brief Decode one exact backend capability profile from durable owner evidence. */
[[nodiscard]] PKGEXEC_API backend_capability_profile
decode_backend_capability_profile(
    const backend_capability_profile_encoding& encoding);

} // namespace pkgexec
