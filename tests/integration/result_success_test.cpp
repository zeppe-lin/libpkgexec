// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

int main()
{
  using namespace pkgexec;

  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  const auto output = stream_capture::retained("stdout\n");
  const auto error_output = stream_capture::retained("stderr\n");
  const auto success = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      request.required_guarantees(), "first diagnostic");
  const auto same_semantics = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      request.required_guarantees(), "different diagnostic");
  TEST_CHECK(success.status() == execution_status::succeeded);
  TEST_CHECK(success.start_state() == execution_start_state::started);
  TEST_CHECK(success.termination()->kind() == process_termination_kind::exited);
  TEST_CHECK(*success.termination()->value() == 0U);
  TEST_CHECK(success.cleanup() == cleanup_outcome::verified);
  TEST_CHECK(!success.failure());
  TEST_CHECK(success.identity() == same_semantics.identity());
  TEST_CHECK(success == same_semantics);

  auto reordered = request.required_guarantees();
  std::reverse(reordered.begin(), reordered.end());
  reordered.push_back(execution_guarantee::exact_interpreter);
  const auto normalized = execution_result::succeeded(
      request, profile, request.interpreter(), output, error_output,
      std::move(reordered));
  TEST_CHECK(normalized.established_guarantees() == request.required_guarantees());
  TEST_CHECK(normalized.identity() == success.identity());

  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::succeeded(
          request, profile, fixture::interpreter("wrong"), output, error_output,
          request.required_guarantees()));
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::succeeded(
          request, profile, request.interpreter(), std::nullopt, error_output,
          request.required_guarantees()));

  auto weak_guarantees = request.required_guarantees();
  weak_guarantees.erase(std::remove(weak_guarantees.begin(), weak_guarantees.end(),
                                    execution_guarantee::network_denied),
                        weak_guarantees.end());
  const auto weak_profile = backend_capability_profile::seal(
      fixture::backend("weak"), weak_guarantees);
  TEST_EXEC_THROWS(error_code::unsupported_request,
      execution_result::succeeded(
          request, weak_profile, request.interpreter(), output, error_output,
          weak_guarantees));
  return EXIT_SUCCESS;
}
