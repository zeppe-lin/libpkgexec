# Testing libpkgexec

The test tree separates evidence by role:

- `unit` pins atomic execution values, identity domains, resource/environment
  normalization, credentials, limits, cancellation state, and backend capability
  profiles without dispatching a backend;
- `integration` composes the real `libpkgsource` program authority with sealed
  execution requests, call-scoped resource admission, backend dispatch, and
  execution-result factories;
- `protocol` qualifies canonical durable backend-profile and execution-result
  bytes independently from ordinary semantic-result tests;
- `header` compiles every public header and the umbrella independently; and
- `contract` checks pkg-config, release, authority, codec, ABI surface/layout,
  provider generation, CI, and test-layout invariants; and
- `installed` contains a consumer compiled only from installed headers and
  pkg-config metadata during release qualification.

`tests/fixtures/execution.h` constructs valid deterministic source/execution
fiction through public APIs. Query/assertion helpers live under `tests/support/`;
backend doubles stay local to the integration tests that exercise them.

The unit suite includes `unit/resource_model_test.cpp` and
`unit/control_test.cpp`. It covers all public identity types, purpose and path
vocabulary, resource slot/access/layout normalization, closed environment
validation, credential and limit normalization, process termination and stream
capture values, monotonic request-bound cancellation, and capability-profile
normalization.

The integration suite includes `integration/request_sealing_test.cpp`,
`integration/resource_admission_test.cpp`, and
`integration/result_guarantee_exactness_test.cpp`. It proves request sensitivity
to the real source-owned program and execution policy, exact concrete-resource
admission, controlled versus uncontrolled backend dispatch, every pre-start and
started failure family, cancellation evidence, and the invariant that result
evidence may retain only guarantees present in the sealed request. A backend
profile may advertise additional capabilities; one result may not claim a
stricter or otherwise different execution contract.

The protocol suite separately qualifies backend-profile owner bytes and
execution-result evidence. Backend-profile tests cover exact canonical round
trip, truncation, checksum/magic/version damage, false identity, noncanonical
guarantee order, and size refusal. Execution-result tests cover success, digest-only
captures, unsupported backend evidence, cancellation before and after start,
nonzero exit, signal termination, resource limits, capture failure, cleanup
failure, checksum damage, truncation, magic/version/shape/evidence corruption,
size bounds, and exact request/backend authority mismatch.

Run the native suite with:

```sh
meson setup --wipe build \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build
meson test -C build --print-errorlogs
```

Role-specific qualification is available with:

```sh
meson test -C build --suite unit --print-errorlogs
meson test -C build --suite integration --print-errorlogs
meson test -C build --suite protocol --print-errorlogs
meson test -C build --suite header --print-errorlogs
meson test -C build --suite contract --print-errorlogs
```

Static qualification requires a separate build:

```sh
meson setup --wipe build-static \
  -Ddefault_library=static \
  -Dlink_mode=static
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

A production Linux executor is intentionally absent from the core. Namespace,
mount, network, credential-transition, pidfd, signal, cgroup, and process
supervision mechanisms are qualified by `libpkgexec-linux`, not simulated here.


## Release-product qualification

`ci/configure-and-test.sh` builds the exact source-3 dependency into an isolated
prefix, builds and tests `libpkgexec` as either a shared or static product,
installs it, and then compiles `tests/installed/consumer.cpp` from the installed
headers and generated pkg-config metadata. The consumer constructs a real
source-owned program, seals and dispatches an execution request, round-trips
execution evidence, and catches a public `pkgexec::error` across the DSO
boundary.

For shared builds, `abi-surface` requires the dynamic C++ symbol set to match
`abi/libpkgexec.exports` exactly. `dependency-abi` requires
`libpkgsource.so.4` and refuses source ABI generations 1 and 2. `abi-layout`
freezes the x86-64 public carriers, including the by-value `pkgsource::program`
inside execution authority. The pkg-config contract requires exactly
`libpkgsource >= 4.0.0, < 5.0.0` and rejects duplicate or stale constraints.

Local release qualification uses `ci/qualify.sh` with `LIBPKGSOURCE_SOURCE` set to the exact qualified source tree; it drives the same installed-product runner for GCC and Clang shared/static builds plus both sanitizer builds.

Hosted CI executes GCC and Clang shared/static builds and a GCC release build.
Separate GCC and Clang jobs run the shared product under ASan+UBSan. The
workflow pins the exact qualified `libpkgsource 3.0.1` source authority rather
than relying on whichever provider happens to be installed on the runner.
