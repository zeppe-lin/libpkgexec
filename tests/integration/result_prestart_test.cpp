// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/guarantees.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

int main()
{
  using namespace pkgexec;

  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  for (const auto failure : {
           execution_failure_kind::request_rejected,
           execution_failure_kind::resource_admission_failed,
           execution_failure_kind::interpreter_unavailable,
           execution_failure_kind::isolation_setup_failed,
           execution_failure_kind::process_start_failed,
       }) {
    const auto result = execution_result::failed_before_start(
        request, profile, failure, {}, "pre-start refusal");
    TEST_CHECK(result.status() == execution_status::failed);
    TEST_CHECK(result.start_state() == execution_start_state::not_started);
    TEST_CHECK(result.failure() == failure);
    TEST_CHECK(!result.termination());
    TEST_CHECK(result.cleanup() == cleanup_outcome::not_required);
  }

  auto weak_guarantees = request.required_guarantees();
  weak_guarantees.erase(std::remove(weak_guarantees.begin(), weak_guarantees.end(),
                                    execution_guarantee::network_denied),
                        weak_guarantees.end());
  const auto weak_profile = backend_capability_profile::seal(
      fixture::backend("weak"), weak_guarantees);
  const auto unsupported = execution_result::failed_before_start(
      request, weak_profile, execution_failure_kind::backend_unsupported);
  TEST_CHECK(unsupported.failure() == execution_failure_kind::backend_unsupported);
  TEST_EXEC_THROWS(error_code::unsupported_request,
      execution_result::failed_before_start(
          request, weak_profile, execution_failure_kind::request_rejected));

  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::program_exited_nonzero));
  TEST_EXEC_THROWS(error_code::invalid_failure,
      execution_result::failed_before_start(
          request, profile, static_cast<execution_failure_kind>(255)));
  TEST_EXEC_THROWS(error_code::invalid_control,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::cancelled));
  TEST_EXEC_THROWS(error_code::inconsistent_result,
      execution_result::failed_before_start(
          request, profile, execution_failure_kind::process_start_failed,
          {execution_guarantee::cleanup_verified}));

  const auto partial = execution_result::failed_before_start(
      request, profile, execution_failure_kind::process_start_failed,
      {execution_guarantee::exact_interpreter,
       execution_guarantee::closed_environment,
       execution_guarantee::root_view});
  TEST_CHECK(partial.established_guarantees().size() == 3U);
  return EXIT_SUCCESS;
}
