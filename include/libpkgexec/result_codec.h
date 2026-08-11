// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file result_codec.h
 *  \brief Versioned durable encoding for native execution evidence.
 */
#pragma once

#include <libpkgexec/export.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <libpkgexec/result.h>

namespace pkgexec {

/*! \brief Current canonical execution-result encoding. */
inline constexpr std::uint16_t execution_result_encoding_version = 1;
/*! \brief Hard refusal bound for one durable execution-result record. */
inline constexpr std::size_t maximum_execution_result_encoding_size =
    64U * 1024U * 1024U;

using execution_result_encoding = std::vector<std::uint8_t>;

/*! \brief Encode exact execution-owned evidence into canonical bytes.
 *
 * The request and backend bodies are not duplicated. Their exact identities
 * are retained in the encoding and must be supplied again during decoding.
 */
[[nodiscard]] PKGEXEC_API execution_result_encoding encode_execution_result(
    const execution_result& result);

/*! \brief Decode evidence under exact caller-supplied semantic authorities.
 *
 * The decoder checks the record checksum, requires the supplied request and
 * backend profile identities, reconstructs the result through its public
 * invariant-enforcing factories, and verifies the retained evidence identity.
 */
[[nodiscard]] PKGEXEC_API execution_result decode_execution_result(
    const execution_result_encoding& encoding,
    execution_request request,
    backend_capability_profile backend);

} // namespace pkgexec
