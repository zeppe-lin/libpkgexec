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
  auto reversed = request.required_guarantees();
  std::reverse(reversed.begin(), reversed.end());
  reversed.push_back(execution_guarantee::exact_interpreter);
  const auto normalized = backend_capability_profile::seal(
      fixture::backend(), std::move(reversed));
  TEST_CHECK(normalized.supports(request));
  TEST_CHECK(std::is_sorted(normalized.guarantees().begin(),
                            normalized.guarantees().end()));
  TEST_CHECK(normalized == fixture::profile(request));

  TEST_EXEC_THROWS(error_code::invalid_capability_profile,
      backend_capability_profile::seal(
          fixture::backend("kind-only-limit-profile"),
          {execution_guarantee::address_space_limit}));
  TEST_EXEC_THROWS(error_code::invalid_capability_profile,
      backend_capability_profile::seal(
          fixture::backend("aggregate-only-limit-profile"),
          {execution_guarantee::resource_limits}));

  auto weak = request.required_guarantees();
  weak.erase(std::remove(weak.begin(), weak.end(),
                         execution_guarantee::network_denied), weak.end());
  TEST_CHECK(!backend_capability_profile::seal(
      fixture::backend("weak"), std::move(weak)).supports(request));
  return EXIT_SUCCESS;
}
