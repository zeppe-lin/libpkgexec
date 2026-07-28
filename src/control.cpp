// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/control.h>

#include <libpkgexec/error.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <utility>

namespace pkgexec {
namespace detail {

class cancellation_state final {
public:
  explicit cancellation_state(execution_request_identity request)
      : request(std::move(request))
  {
  }

  execution_request_identity request;
  std::atomic<bool> requested{false};
  std::mutex mutex;
  std::condition_variable changed;
};

} // namespace detail

cancellation_token::cancellation_token(
    std::shared_ptr<detail::cancellation_state> state)
    : state_(std::move(state))
{
}

const execution_request_identity& cancellation_token::request_identity() const noexcept
{
  return state_->request;
}

bool cancellation_token::applies_to(const execution_request& request) const noexcept
{
  return request.identity() == state_->request;
}

bool cancellation_token::cancellation_requested() const noexcept
{
  return state_->requested.load(std::memory_order_acquire);
}

void cancellation_token::wait() const
{
  std::unique_lock<std::mutex> lock(state_->mutex);
  state_->changed.wait(lock, [this] {
    return state_->requested.load(std::memory_order_acquire);
  });
}

bool cancellation_token::wait_for(std::chrono::milliseconds timeout) const
{
  if (timeout <= std::chrono::milliseconds::zero()) {
    return cancellation_requested();
  }
  std::unique_lock<std::mutex> lock(state_->mutex);
  return state_->changed.wait_for(lock, timeout, [this] {
    return state_->requested.load(std::memory_order_acquire);
  });
}

cancellation_source::cancellation_source(
    std::shared_ptr<detail::cancellation_state> state)
    : state_(std::move(state))
{
}

cancellation_source cancellation_source::for_request(
    const execution_request& request)
{
  if (request.cancellation().mode() == cancellation_mode::disabled) {
    throw error(error_code::invalid_control,
                "disabled cancellation has no call-scoped control authority");
  }
  return cancellation_source(
      std::make_shared<detail::cancellation_state>(request.identity()));
}

const execution_request_identity& cancellation_source::request_identity() const noexcept
{
  return state_->request;
}

cancellation_token cancellation_source::token() const noexcept
{
  return cancellation_token(state_);
}

bool cancellation_source::request_cancellation()
{
  {
    std::lock_guard<std::mutex> lock(state_->mutex);
    if (state_->requested.load(std::memory_order_relaxed)) {
      return false;
    }
    state_->requested.store(true, std::memory_order_release);
  }
  state_->changed.notify_all();
  return true;
}

bool cancellation_source::cancellation_requested() const noexcept
{
  return state_->requested.load(std::memory_order_acquire);
}

void require_cancellation_control(const execution_request& request,
                                  const cancellation_token& token)
{
  if (request.cancellation().mode() == cancellation_mode::disabled) {
    throw error(error_code::invalid_control,
                "controlled execution requires an enabled cancellation policy");
  }
  if (!token.applies_to(request)) {
    throw error(error_code::control_mismatch,
                "cancellation control does not match the sealed execution request");
  }
}

} // namespace pkgexec
