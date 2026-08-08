// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/test.h"

#include <libpkgexec/libpkgexec.h>

#include <string>

int main()
{
  using namespace pkgexec;
  const std::string zero(64, '0');

  TEST_CHECK(execution_request_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(environment_policy_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(resource_layout_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(credential_policy_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(resource_limits_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(root_view_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(resource_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(interpreter_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(backend_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(backend_capability_profile_identity::from_sha256(zero).hex() == zero);
  TEST_CHECK(execution_evidence_identity::from_sha256(zero).hex() == zero);

  TEST_EXEC_THROWS(error_code::invalid_identity,
                   execution_request_identity::from_sha256("bad"));
  TEST_EXEC_THROWS(error_code::invalid_identity,
                   backend_identity::from_sha256(std::string(64, 'A')));
  TEST_EXEC_THROWS(error_code::invalid_identity,
                   resource_identity::from_sha256(std::string(63, '0')));

  TEST_CHECK(sha256_digest::of_bytes("").hex() ==
             "e3b0c44298fc1c149afbf4c8996fb924"
             "27ae41e4649b934ca495991b7852b855");
  TEST_CHECK(sha256_digest::of_bytes("a") != sha256_digest::of_bytes("b"));
  return EXIT_SUCCESS;
}
