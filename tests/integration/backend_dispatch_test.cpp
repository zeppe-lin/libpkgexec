// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

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
      : profile_(std::move(profile)) {}
  pkgexec::backend_capability_profile capabilities() const override
  { return profile_; }
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
      : profile_(std::move(profile)) {}
  pkgexec::backend_capability_profile capabilities() const override
  { return profile_; }
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

  const auto ordinary_request = fixture::uncontrolled_request();
  const auto ordinary_resources = execution_resources::admit(
      ordinary_request, ordinary_request.root_view(), "/host/root",
      fixture::materializations());
  fake_backend ordinary_backend(fixture::profile(ordinary_request));
  TEST_CHECK(ordinary_backend.capabilities().supports(ordinary_request));
  TEST_CHECK(ordinary_backend.execute(ordinary_request, ordinary_resources).status() ==
             execution_status::succeeded);

  const auto controlled_request = fixture::request();
  const auto controlled_resources = execution_resources::admit(
      controlled_request, controlled_request.root_view(), "/host/root",
      fixture::materializations());
  fake_controlled_backend controlled(fixture::profile(controlled_request));
  TEST_EXEC_THROWS(error_code::invalid_control,
                   controlled.execute(controlled_request, controlled_resources));

  auto cancellation = cancellation_source::for_request(controlled_request);
  TEST_CHECK(controlled.execute(controlled_request, controlled_resources,
                                cancellation.token()).status() ==
             execution_status::succeeded);

  const auto other_request = fixture::request_with_cancellation(
      cancellation_policy::graceful_then_forced(700));
  auto other_cancellation = cancellation_source::for_request(other_request);
  TEST_EXEC_THROWS(error_code::control_mismatch,
      controlled.execute(controlled_request, controlled_resources,
                         other_cancellation.token()));

  TEST_CHECK(cancellation.request_cancellation());
  const auto cancelled = controlled.execute(
      controlled_request, controlled_resources, cancellation.token());
  TEST_CHECK(cancelled.start_state() == execution_start_state::not_started);
  TEST_CHECK(cancelled.failure() == execution_failure_kind::cancelled);
  return EXIT_SUCCESS;
}
