// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#include "test.h"

#include <libpkgexec/libpkgexec.h>

#include <algorithm>

namespace {

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
    return pkgexec::execution_result::succeeded(
        request, profile_, request.interpreter(),
        pkgexec::stream_capture::retained("ok\n"),
        pkgexec::stream_capture::retained(""),
        request.required_guarantees());
  }
private:
  pkgexec::backend_capability_profile profile_;
};

} // namespace

int main()
{
  using namespace pkgexec;
  const auto request = fixture::request();
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

  return EXIT_SUCCESS;
}
