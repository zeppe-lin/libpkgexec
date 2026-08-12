// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/execution.h"
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

int main()
{
  using namespace pkgexec;
  const auto request = fixture::request();
  const auto profile = fixture::profile(request);
  const auto encoding = encode_backend_capability_profile(profile);
  const auto decoded = decode_backend_capability_profile(encoding);

  TEST_CHECK(decoded == profile);
  TEST_CHECK(decoded.backend() == profile.backend());
  TEST_CHECK(decoded.guarantees() == profile.guarantees());
  TEST_CHECK(encode_backend_capability_profile(decoded) == encoding);
  TEST_CHECK(encoding.size() < maximum_backend_capability_profile_encoding_size);
  return EXIT_SUCCESS;
}
