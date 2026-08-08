// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdlib>
#include <iostream>

#define TEST_CHECK(expression)                                                 \
  do {                                                                         \
    if (!(expression)) {                                                       \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": check failed: " #expression << '\n';                   \
      return EXIT_FAILURE;                                                     \
    }                                                                          \
  } while (false)

#define TEST_EXEC_THROWS(expected_code, expression)                            \
  do {                                                                         \
    bool caught_ = false;                                                       \
    try {                                                                      \
      (void)(expression);                                                       \
    } catch (const pkgexec::error& error_) {                                    \
      caught_ = true;                                                           \
      if (error_.code() != (expected_code)) {                                  \
        std::cerr << __FILE__ << ':' << __LINE__                               \
                  << ": unexpected error code\n";                             \
        return EXIT_FAILURE;                                                    \
      }                                                                         \
    }                                                                           \
    if (!caught_) {                                                             \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": expected pkgexec::error\n";                            \
      return EXIT_FAILURE;                                                      \
    }                                                                           \
  } while (false)
