#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=${1:?source root required}
header=$root/include/libpkgexec/profile_codec.h
source=$root/src/profile_codec.cpp
round_trip_test=$root/tests/protocol/backend_profile_codec_roundtrip_test.cpp
corruption_test=$root/tests/protocol/backend_profile_codec_corruption_test.cpp
manual=$root/docs/man/pkgexec_profile_codec.3.md

for file in "$header" "$source" "$round_trip_test" "$corruption_test" "$manual"; do
  [ -s "$file" ] || {
    echo "profile-codec-contract: missing source: $file" >&2
    exit 1
  }
done


for registration in \
  "$root/include/libpkgexec/libpkgexec.h:#include <libpkgexec/profile_codec.h>" \
  "$root/src/meson.build:'profile_codec.cpp'" \
  "$root/src/meson.build:../include/libpkgexec/profile_codec.h" \
  "$root/tests/meson.build:'profile_codec.h'" \
  "$root/tests/meson.build:'profile_codec_contract'" \
  "$root/docs/man/meson.build:pkgexec_profile_codec.3"; do
  file=${registration%%:*}
  token=${registration#*:}
  grep -F "$token" "$file" >/dev/null || {
    echo "profile-codec-contract: missing registration: $token" >&2
    exit 1
  }
done

for token in \
  'backend_capability_profile_encoding_version = 1' \
  'maximum_backend_capability_profile_encoding_size' \
  'backend_capability_profile_encoding' \
  'encode_backend_capability_profile' \
  'decode_backend_capability_profile'; do
  grep -F "$token" "$header" >/dev/null || {
    echo "profile-codec-contract: missing public token: $token" >&2
    exit 1
  }
done

for token in \
  'backend-capability-profile encoding checksum mismatch' \
  'backend-capability-profile identity mismatch' \
  'backend_capability_profile::seal' \
  'encode_backend_capability_profile(decoded)' \
  'backend-capability-profile encoding is not canonical'; do
  grep -F "$token" "$source" >/dev/null || {
    echo "profile-codec-contract: missing implementation token: $token" >&2
    exit 1
  }
done

for token in \
  'encode_backend_capability_profile(decoded) == encoding' \
  'decoded == profile'; do
  grep -F "$token" "$round_trip_test" >/dev/null || {
    echo "profile-codec-contract: missing round-trip token: $token" >&2
    exit 1
  }
done
for token in 'corrupt_encoding' 'refresh_checksum' 'noncanonical' 'oversized'; do
  grep -F "$token" "$corruption_test" >/dev/null || {
    echo "profile-codec-contract: missing refusal token: $token" >&2
    exit 1
  }
done

if grep -Eq '\.execute\(|execution_request|execution_result' "$source"; then
  echo 'profile-codec-contract: execution machinery leaked into profile codec' >&2
  exit 1
fi
if grep -Eq 'opendir\(|readdir\(|open\(|stat\(|filesystem' "$source"; then
  echo 'profile-codec-contract: host observation leaked into profile codec' >&2
  exit 1
fi
