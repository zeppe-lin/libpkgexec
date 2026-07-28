// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

namespace {

pkgexec::execution_result success(
    const pkgexec::execution_request& request,
    const pkgexec::backend_capability_profile& profile)
{
  return pkgexec::execution_result::succeeded(
      request, profile, request.interpreter(),
      pkgexec::stream_capture::retained("ok\n"),
      pkgexec::stream_capture::retained(""),
      request.required_guarantees());
}

class fake_backend final : public pkgexec::execution_backend {
public:
  explicit fake_backend(pkgexec::backend_capability_profile profile)
      : profile_(std::move(profile))
  {
  }

  pkgexec::backend_capability_profile capabilities() const override
  {
    return profile_;
  }

  pkgexec::execution_result execute(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources& resources) override
  {
    (void)resources.materialization(fixture::resource("workspace"));
    return success(request, profile_);
  }
private:
  pkgexec::backend_capability_profile profile_;
};

class fake_controlled_backend final : public pkgexec::controlled_execution_backend {
public:
  explicit fake_controlled_backend(pkgexec::backend_capability_profile profile)
      : profile_(std::move(profile))
  {
  }

  pkgexec::backend_capability_profile capabilities() const override
  {
    return profile_;
  }
private:
  pkgexec::execution_result execute_uncontrolled(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources& resources) override
  {
    (void)resources.materialization(fixture::resource("workspace"));
    return success(request, profile_);
  }

  pkgexec::execution_result execute_controlled(
      const pkgexec::execution_request& request,
      const pkgexec::execution_resources& resources,
      const pkgexec::cancellation_token& cancellation) override
  {
    (void)resources.materialization(fixture::resource("workspace"));
    if (cancellation.cancellation_requested()) {
      return pkgexec::execution_result::cancelled_before_start(
          request, profile_, cancellation,
          {pkgexec::execution_guarantee::cancellation});
    }
    return success(request, profile_);
  }

  pkgexec::backend_capability_profile profile_;
};

} // namespace

int main()
{
  using namespace pkgexec;
  const auto request = fixture::uncontrolled_request();
  auto materializations = fixture::materializations();
  std::reverse(materializations.begin(), materializations.end());
  const auto resources = execution_resources::admit(
      request, request.root_view(), "/host/root", std::move(materializations));
  TEST_CHECK(resources.root_view() == request.root_view());
  TEST_CHECK(resources.materializations().size() == 4);
  TEST_CHECK(resources.materialization(fixture::resource("source-main"))
                 .host_path() == "/host/source");

  TEST_EXEC_THROWS(error_code::resource_mismatch,
      execution_resources::admit(
          request, fixture::root("wrong"), "/host/root",
          fixture::materializations()));

  auto missing = fixture::materializations();
  missing.pop_back();
  TEST_EXEC_THROWS(error_code::resource_mismatch,
      execution_resources::admit(
          request, request.root_view(), "/host/root", std::move(missing)));

  auto duplicate = fixture::materializations();
  duplicate.push_back(
      resource_materialization(fixture::resource("workspace"), "/other"));
  TEST_EXEC_THROWS(error_code::duplicate_resource,
      execution_resources::admit(
          request, request.root_view(), "/host/root", std::move(duplicate)));

  fake_backend backend(fixture::profile(request));
  TEST_CHECK(backend.capabilities().supports(request));
  const auto result = backend.execute(request, resources);
  TEST_CHECK(result.status() == execution_status::succeeded);
  TEST_CHECK(result.standard_output()->material() == std::optional<std::string>("ok\n"));

  fake_controlled_backend controlled(fixture::profile(fixture::request()));
  const auto ordinary = controlled.execute(request, resources);
  TEST_CHECK(ordinary.status() == execution_status::succeeded);

  const auto controlled_request = fixture::request();
  const auto controlled_resources = execution_resources::admit(
      controlled_request, controlled_request.root_view(), "/host/root",
      fixture::materializations());
  TEST_EXEC_THROWS(error_code::invalid_control,
                   controlled.execute(controlled_request, controlled_resources));

  auto cancellation = cancellation_source::for_request(controlled_request);
  const auto completed = controlled.execute(
      controlled_request, controlled_resources, cancellation.token());
  TEST_CHECK(completed.status() == execution_status::succeeded);

  const auto other_request = fixture::request_with_cancellation(
      cancellation_policy::graceful_then_forced(700));
  auto other_cancellation = cancellation_source::for_request(other_request);
  TEST_EXEC_THROWS(error_code::control_mismatch,
      controlled.execute(
          controlled_request, controlled_resources, other_cancellation.token()));

  TEST_CHECK(cancellation.request_cancellation());
  const auto cancelled = controlled.execute(
      controlled_request, controlled_resources, cancellation.token());
  TEST_CHECK(cancelled.status() == execution_status::failed);
  TEST_CHECK(cancelled.start_state() == execution_start_state::not_started);
  TEST_CHECK(cancelled.failure() == execution_failure_kind::cancelled);
  TEST_CHECK(!cancelled.termination());

  return EXIT_SUCCESS;
}
