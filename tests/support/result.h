// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgexec/libpkgexec.h>

namespace test_support {

inline bool exact_result(const pkgexec::execution_result& lhs,
                         const pkgexec::execution_result& rhs)
{
  return lhs.status() == rhs.status() &&
         lhs.start_state() == rhs.start_state() &&
         lhs.request() == rhs.request() &&
         lhs.backend() == rhs.backend() &&
         lhs.observed_interpreter() == rhs.observed_interpreter() &&
         lhs.termination() == rhs.termination() &&
         lhs.standard_output() == rhs.standard_output() &&
         lhs.standard_error() == rhs.standard_error() &&
         lhs.established_guarantees() == rhs.established_guarantees() &&
         lhs.cleanup() == rhs.cleanup() &&
         lhs.failure() == rhs.failure() &&
         lhs.diagnostic() == rhs.diagnostic() &&
         lhs.identity() == rhs.identity();
}

} // namespace test_support
