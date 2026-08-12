// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/libpkgexec.h>

#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

pkgexec::resource_identity resource(std::string_view value)
{
  return pkgexec::resource_identity::from_sha256(
      pkgexec::sha256_digest::of_bytes(value).hex());
}

pkgexec::execution_request request()
{
  using namespace pkgexec;
  std::vector<resource_binding> bindings;
  bindings.emplace_back(
      resource_slot::named(resource_role::source_tree, "main"),
      resource("source"), resource_access::read_only,
      logical_path::parse("/src"));
  bindings.emplace_back(
      resource_slot::singleton(resource_role::build_workspace),
      resource("workspace"), resource_access::writable,
      logical_path::parse("/build"));
  auto layout = resource_layout::seal(
      std::move(bindings),
      resource_slot::singleton(resource_role::build_workspace));
  auto environment = environment_policy::hermetic(
      {logical_path::parse("/usr/bin"), logical_path::parse("/bin")},
      logical_path::parse("/home/build"), logical_path::parse("/tmp"), 2);
  return execution_request::seal(
      pkgsource::program(pkgsource::program_language::posix_shell, "true\n"),
      execution_purpose::build(),
      interpreter_identity::from_sha256(
          sha256_digest::of_bytes("/bin/sh").hex()),
      root_view_identity::from_sha256(
          sha256_digest::of_bytes("root").hex()),
      std::move(layout), std::move(environment),
      credential_policy::fixed(1000, 1000), resource_limits::make(),
      cancellation_policy::disabled());
}

class success_backend final : public pkgexec::execution_backend {
public:
  explicit success_backend(pkgexec::backend_capability_profile profile)
      : profile_(std::move(profile))
  {
  }

  pkgexec::backend_capability_profile capabilities() const override
  {
    return profile_;
  }

  pkgexec::execution_result execute(
      const pkgexec::execution_request& value,
      const pkgexec::execution_resources&) override
  {
    return pkgexec::execution_result::succeeded(
        value, profile_, value.interpreter(), pkgexec::stream_capture::retained(""),
        pkgexec::stream_capture::retained(""), value.required_guarantees());
  }

private:
  pkgexec::backend_capability_profile profile_;
};

} // namespace

int main()
{
  using namespace pkgexec;
  const auto sealed = request();
  auto profile = backend_capability_profile::seal(
      backend_identity::from_sha256(sha256_digest::of_bytes("backend").hex()),
      sealed.required_guarantees());
  auto resources = execution_resources::admit(
      sealed, sealed.root_view(), "/host/root",
      {resource_materialization(resource("source"), "/host/source"),
       resource_materialization(resource("workspace"), "/host/work")});
  success_backend backend(profile);
  const auto result = backend.execute(sealed, resources);
  if (result.status() != execution_status::succeeded ||
      result.request().identity() != sealed.identity())
    return EXIT_FAILURE;

  const auto encoded_profile = encode_backend_capability_profile(profile);
  const auto decoded_profile =
      decode_backend_capability_profile(encoded_profile);
  if (decoded_profile != profile)
    return EXIT_FAILURE;

  const auto encoded = encode_execution_result(result);
  const auto decoded = decode_execution_result(
      encoded, sealed, decoded_profile);
  if (decoded.identity() != result.identity())
    return EXIT_FAILURE;

  try {
    (void)resource_slot::singleton(static_cast<resource_role>(255));
  } catch (const error& value) {
    return value.code() == error_code::invalid_value ? EXIT_SUCCESS : 2;
  }
  return 3;
}
