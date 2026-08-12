# Maintaining libpkgexec

## Invariants

- The source-owned program remains exact and uninterpreted.
- Concrete host paths never enter semantic identities.
- The process environment is closed and explicit.
- Unsupported guarantees are refused before process start.
- Unsupported enum vocabulary is refused before it enters semantic identity or execution evidence.
- Resource-limit support is advertised and retained by exact requested kind.
- Cancellation control is request-bound, monotonic, and absent from identities.
- Enabled cancellation never enters a controlled backend through the ordinary path.
- Result evidence may retain only guarantees present in the sealed request.
- Success requires exactly the requested guarantees and verified cleanup.
- Diagnostic observations do not silently become semantic identity material.
- Result records never reconstruct requests or backend profiles from identities.
- Backend profile records reconstruct only the exact profile body they own:
  backend identity plus canonical guarantees, never a live backend.
- Durable result decoding verifies checksum before authority and semantic admission.
- The core remains free of process syscalls and Linux isolation mechanisms.
- Build, lifecycle, apply, state, transaction, and controller semantics remain
  outside this repository.

## ABI and compatibility

`libpkgexec 2.0.0` is the first release with a reviewed ELF export manifest.
The shared ABI is `libpkgexec.so.2`; `abi/libpkgexec.exports` is the authoritative
reviewed symbol surface for that generation. Public carrier layouts are part of
the C++ ABI even when an outer `sizeof` remains unchanged because variant or
foreign alternatives may change internally.

`execution_request` retains `pkgsource::program` by value. A future
`libpkgsource` ABI generation therefore requires explicit layout and semantic
review before widening the accepted `>= 3.0.1, < 4.0.0` interval. Do not add
legacy execution records, compatibility aliases, or an in-library old-ABI shim.

## Release checklist

1. Run `unit`, `integration`, `protocol`, `header`, and `contract` suites in both shared and static builds.
2. Compile every public header standalone.
3. Run the installed consumer against both installed shared and static products.
4. Run GCC and Clang warnings-as-errors qualification.
5. Run GCC and Clang ASan+UBSan qualification.
6. Compare the shared dynamic symbol set exactly with `abi/libpkgexec.exports`.
7. Inspect the SONAME and require direct `NEEDED libpkgsource.so.3`; refuse obsolete source generations.
8. Verify generated pkg-config metadata contains the exact source-3 interval once.
9. Run `git diff --check` and `git fsck`.
10. Render and lint manuals where scdoc and mandoc are available.
11. Confirm `HISTORY.md`, package version, SONAME, dependency interval, ABI manifest, and CI pin agree.
