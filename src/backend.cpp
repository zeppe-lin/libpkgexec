// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgexec/backend.h>

#include <libpkgexec/error.h>

#include <algorithm>
#include <set>
#include <utility>

namespace pkgexec {

resource_materialization::resource_materialization(
    resource_identity resource,
    std::filesystem::path host_path)
    : resource_(std::move(resource)), host_path_(std::move(host_path))
{
  if (host_path_.empty() || !host_path_.is_absolute()) {
    throw error(error_code::invalid_path,
                "concrete execution resource paths must be absolute");
  }
  host_path_ = host_path_.lexically_normal();
}
const resource_identity& resource_materialization::resource() const noexcept
{ return resource_; }
const std::filesystem::path& resource_materialization::host_path() const noexcept
{ return host_path_; }

execution_resources::execution_resources(
    root_view_identity root_view,
    std::filesystem::path root_view_path,
    std::vector<resource_materialization> materializations)
    : root_view_(std::move(root_view)),
      root_view_path_(std::move(root_view_path)),
      materializations_(std::move(materializations))
{
}
execution_resources execution_resources::admit(
    const execution_request& request,
    root_view_identity root_view,
    std::filesystem::path root_view_path,
    std::vector<resource_materialization> materializations)
{
  if (root_view != request.root_view()) {
    throw error(error_code::resource_mismatch,
                "concrete root view does not match the sealed request");
  }
  if (root_view_path.empty() || !root_view_path.is_absolute()) {
    throw error(error_code::invalid_path,
                "concrete root-view path must be absolute");
  }
  root_view_path = root_view_path.lexically_normal();
  std::sort(materializations.begin(), materializations.end(),
      [](const resource_materialization& lhs,
         const resource_materialization& rhs) {
        return lhs.resource() < rhs.resource();
      });
  for (std::size_t i = 1; i < materializations.size(); ++i) {
    if (materializations[i - 1].resource() == materializations[i].resource()) {
      throw error(error_code::duplicate_resource,
                  "duplicate concrete execution resource identity");
    }
  }

  std::vector<resource_identity> required;
  required.reserve(request.resources().bindings().size());
  for (const auto& binding : request.resources().bindings()) {
    required.push_back(binding.resource());
  }
  std::sort(required.begin(), required.end());
  required.erase(std::unique(required.begin(), required.end()), required.end());
  if (required.size() != materializations.size()) {
    throw error(error_code::resource_mismatch,
                "concrete execution resources do not exactly match the request");
  }
  for (std::size_t i = 0; i < required.size(); ++i) {
    if (required[i] != materializations[i].resource()) {
      throw error(error_code::resource_mismatch,
                  "concrete execution resource identity does not match the request");
    }
  }
  return execution_resources(std::move(root_view), std::move(root_view_path),
                             std::move(materializations));
}
const root_view_identity& execution_resources::root_view() const noexcept
{ return root_view_; }
const std::filesystem::path& execution_resources::root_view_path() const noexcept
{ return root_view_path_; }
const std::vector<resource_materialization>& execution_resources::materializations() const noexcept
{ return materializations_; }
const resource_materialization& execution_resources::materialization(
    const resource_identity& resource) const
{
  const auto found = std::lower_bound(
      materializations_.begin(), materializations_.end(), resource,
      [](const resource_materialization& value, const resource_identity& key) {
        return value.resource() < key;
      });
  if (found == materializations_.end() || found->resource() != resource) {
    throw error(error_code::resource_mismatch,
                "unknown concrete execution resource identity");
  }
  return *found;
}

execution_result controlled_execution_backend::execute(
    const execution_request& request,
    const execution_resources& resources)
{
  if (request.cancellation().mode() != cancellation_mode::disabled) {
    throw error(error_code::invalid_control,
                "enabled cancellation requires controlled backend execution");
  }
  return execute_uncontrolled(request, resources);
}

execution_result controlled_execution_backend::execute(
    const execution_request& request,
    const execution_resources& resources,
    const cancellation_token& cancellation)
{
  require_cancellation_control(request, cancellation);
  return execute_controlled(request, resources, cancellation);
}

} // namespace pkgexec
