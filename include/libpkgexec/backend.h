// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file backend.h
 *  \brief Call-scoped resource admission and executor contract.
 */
#pragma once

#include <filesystem>
#include <vector>

#include <libpkgexec/result.h>

namespace pkgexec {

/*! \brief Concrete host resource for one semantic request resource. */
class resource_materialization final {
public:
  resource_materialization(resource_identity resource,
                           std::filesystem::path host_path);
  [[nodiscard]] const resource_identity& resource() const noexcept;
  [[nodiscard]] const std::filesystem::path& host_path() const noexcept;
private:
  resource_identity resource_;
  std::filesystem::path host_path_;
};

/*! \brief Exact call-scoped resources admitted against one request. */
class execution_resources final {
public:
  [[nodiscard]] static execution_resources admit(
      const execution_request& request,
      root_view_identity root_view,
      std::filesystem::path root_view_path,
      std::vector<resource_materialization> materializations);
  [[nodiscard]] const root_view_identity& root_view() const noexcept;
  [[nodiscard]] const std::filesystem::path& root_view_path() const noexcept;
  [[nodiscard]] const std::vector<resource_materialization>& materializations() const noexcept;
  [[nodiscard]] const resource_materialization& materialization(
      const resource_identity& resource) const;
private:
  execution_resources(root_view_identity root_view,
                      std::filesystem::path root_view_path,
                      std::vector<resource_materialization> materializations);
  root_view_identity root_view_;
  std::filesystem::path root_view_path_;
  std::vector<resource_materialization> materializations_;
};

/*! \brief Abstract executor boundary; the core performs no process syscalls. */
class execution_backend {
public:
  virtual ~execution_backend() = default;
  [[nodiscard]] virtual backend_capability_profile capabilities() const = 0;
  [[nodiscard]] virtual execution_result execute(
      const execution_request& request,
      const execution_resources& resources) = 0;
};

} // namespace pkgexec
