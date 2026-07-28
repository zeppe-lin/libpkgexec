// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file control.h
 *  \brief Call-scoped cancellation control for controlled execution.
 */
#pragma once

#include <chrono>
#include <memory>

#include <libpkgexec/request.h>

namespace pkgexec {
namespace detail {
class cancellation_state;
}

/*! \brief Copyable read-only view of one request-bound cancellation state. */
class cancellation_token final {
public:
  cancellation_token(const cancellation_token&) noexcept = default;
  cancellation_token& operator=(const cancellation_token&) noexcept = default;

  [[nodiscard]] const execution_request_identity& request_identity() const noexcept;
  [[nodiscard]] bool applies_to(const execution_request& request) const noexcept;
  [[nodiscard]] bool cancellation_requested() const noexcept;
  void wait() const;
  [[nodiscard]] bool wait_for(std::chrono::milliseconds timeout) const;
private:
  friend class cancellation_source;
  explicit cancellation_token(std::shared_ptr<detail::cancellation_state> state);
  std::shared_ptr<detail::cancellation_state> state_;
};

/*! \brief Unique caller-owned authority for one monotonic cancellation signal. */
class cancellation_source final {
public:
  [[nodiscard]] static cancellation_source for_request(
      const execution_request& request);

  cancellation_source(cancellation_source&&) noexcept = default;
  cancellation_source& operator=(cancellation_source&&) noexcept = default;
  cancellation_source(const cancellation_source&) = delete;
  cancellation_source& operator=(const cancellation_source&) = delete;

  [[nodiscard]] const execution_request_identity& request_identity() const noexcept;
  [[nodiscard]] cancellation_token token() const noexcept;
  [[nodiscard]] bool request_cancellation();
  [[nodiscard]] bool cancellation_requested() const noexcept;
private:
  explicit cancellation_source(std::shared_ptr<detail::cancellation_state> state);
  std::shared_ptr<detail::cancellation_state> state_;
};

/*! \brief Validate that a token is the control admitted for this request. */
void require_cancellation_control(const execution_request& request,
                                  const cancellation_token& token);

} // namespace pkgexec
