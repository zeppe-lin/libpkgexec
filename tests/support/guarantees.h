// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgexec/libpkgexec.h>

#include <algorithm>
#include <vector>

namespace test_support {

inline bool has_guarantee(
    const std::vector<pkgexec::execution_guarantee>& values,
    pkgexec::execution_guarantee value)
{
  return std::binary_search(values.begin(), values.end(), value);
}

inline std::vector<pkgexec::execution_guarantee> without_guarantee(
    std::vector<pkgexec::execution_guarantee> values,
    pkgexec::execution_guarantee value)
{
  values.erase(std::remove(values.begin(), values.end(), value), values.end());
  return values;
}

inline std::vector<pkgexec::execution_guarantee> with_guarantee(
    std::vector<pkgexec::execution_guarantee> values,
    pkgexec::execution_guarantee value)
{
  values.push_back(value);
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

} // namespace test_support
