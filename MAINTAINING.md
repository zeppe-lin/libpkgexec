# Maintaining libpkgexec

## Invariants

- The source-owned program remains exact and uninterpreted.
- Concrete host paths never enter semantic identities.
- The process environment is closed and explicit.
- Unsupported guarantees are refused before process start.
- Resource-limit support is advertised and retained by exact requested kind.
- Cancellation control is request-bound, monotonic, and absent from identities.
- Enabled cancellation never enters a controlled backend through the ordinary path.
- Result evidence may retain only guarantees present in the sealed request.
- Success requires exactly the requested guarantees and verified cleanup.
- Diagnostic observations do not silently become semantic identity material.
- Result records never reconstruct requests or backend profiles from identities.
- Durable result decoding verifies checksum before authority and semantic admission.
- The core remains free of process syscalls and Linux isolation mechanisms.
- Build, lifecycle, apply, state, transaction, and controller semantics remain
  outside this repository.

## Compatibility

Public API or identity-domain changes require an ABI review. Do not add legacy
execution records or compatibility aliases to preserve provisional consumers.
Migration belongs in external adapters.

## Release checklist

1. Run `unit`, `integration`, `protocol`, `header`, and `contract` suites in both shared and static builds.
2. Compile every public header standalone.
3. Run GCC and Clang warnings-as-errors checks.
4. Run ASan and UBSan tests.
5. Inspect SONAME and direct `NEEDED` dependencies.
6. Run `git diff --check` and `git fsck`.
7. Render and lint manuals where scdoc and mandoc are available.
8. Confirm `HISTORY.md`, package version, SONAME, and pkg-config floors agree.
