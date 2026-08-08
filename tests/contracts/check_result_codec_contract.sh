#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
header=$root/include/libpkgexec/result_codec.h
source=$root/src/result_codec.cpp
round_trip_test=$root/tests/protocol/result_codec_roundtrip_test.cpp
corruption_test=$root/tests/protocol/result_codec_corruption_test.cpp

for token in \
  'execution_result_encoding_version = 1' \
  'maximum_execution_result_encoding_size' \
  'execution_request request' \
  'backend_capability_profile backend'
do
  grep -q "$token" "$header" || {
    echo "result-codec-contract: missing public token: $token" >&2
    exit 1
  }
done

for token in \
  'execution-result encoding checksum mismatch' \
  'execution-result record belongs to another execution request' \
  'execution-result record belongs to another backend profile' \
  'execution_result::succeeded' \
  'execution_result::failed_before_start' \
  'execution_result::cancelled_before_start' \
  'execution_result::failed_after_start' \
  'execution_result::cancelled_after_start' \
  'encode_execution_result(decoded)' \
  'execution-result evidence identity mismatch'
do
  grep -q "$token" "$source" || {
    echo "result-codec-contract: missing implementation token: $token" >&2
    exit 1
  }
done

checksum_line=$(grep -n 'checksum mismatch' "$source" | head -n1 | cut -d: -f1)
request_line=$(grep -n 'belongs to another execution request' "$source" | head -n1 | cut -d: -f1)
[ -n "$checksum_line" ] && [ -n "$request_line" ] && \
  [ "$checksum_line" -lt "$request_line" ] || {
  echo 'result-codec-contract: checksum is not admitted before authority fields' >&2
  exit 1
}

if grep -Eq '\.program\(\)|\.resources\(\)|\.credentials\(\)|\.environment\(\)' "$source"; then
  echo 'result-codec-contract: semantic request serialization leaked into result codec' >&2
  exit 1
fi

for token in \
  'round_trip(success)' \
  'execution_result::cancelled_before_start' \
  'execution_result::cancelled_after_start' \
  'execution_failure_kind::log_capture_failed' \
  'execution_failure_kind::cleanup_failed'
do
  grep -q "$token" "$round_trip_test" || {
    echo "result-codec-contract: missing round-trip test token: $token" >&2
    exit 1
  }
done
for token in \
  'error_code::corrupt_encoding' \
  'error_code::authority_mismatch' \
  'refresh_checksum'
do
  grep -q "$token" "$corruption_test" || {
    echo "result-codec-contract: missing corruption test token: $token" >&2
    exit 1
  }
done
