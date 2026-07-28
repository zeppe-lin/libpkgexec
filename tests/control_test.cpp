// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

#include <atomic>
#include <chrono>
#include <thread>

int main()
{
  using namespace pkgexec;
  using namespace std::chrono_literals;

  const auto request = fixture::request();
  const auto other_request = fixture::request_with_cancellation(
      cancellation_policy::graceful_then_forced(700));
  const auto disabled = fixture::uncontrolled_request();

  TEST_EXEC_THROWS(error_code::invalid_control,
                   cancellation_source::for_request(disabled));

  auto source = cancellation_source::for_request(request);
  const auto token = source.token();
  const auto copied = token;
  TEST_CHECK(source.request_identity() == request.identity());
  TEST_CHECK(token.request_identity() == request.identity());
  TEST_CHECK(token.applies_to(request));
  TEST_CHECK(!token.applies_to(other_request));
  TEST_CHECK(!source.cancellation_requested());
  TEST_CHECK(!token.cancellation_requested());
  TEST_CHECK(!token.wait_for(0ms));
  require_cancellation_control(request, token);
  TEST_EXEC_THROWS(error_code::control_mismatch,
                   require_cancellation_control(other_request, token));
  TEST_EXEC_THROWS(error_code::invalid_control,
                   require_cancellation_control(disabled, token));

  std::atomic<bool> waiting{false};
  std::atomic<bool> observed{false};
  std::thread waiter([&] {
    waiting.store(true, std::memory_order_release);
    copied.wait();
    observed.store(true, std::memory_order_release);
  });
  while (!waiting.load(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  TEST_CHECK(source.request_cancellation());
  TEST_CHECK(!source.request_cancellation());
  waiter.join();

  TEST_CHECK(observed.load(std::memory_order_acquire));
  TEST_CHECK(source.cancellation_requested());
  TEST_CHECK(token.cancellation_requested());
  TEST_CHECK(copied.wait_for(0ms));

  return EXIT_SUCCESS;
}
