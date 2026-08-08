# History

## libpkgexec 1.4.0

Durable execution-evidence codec release.

- Confined established execution guarantees to the exact sealed request; a
  wider backend capability profile no longer permits one result to claim an
  unrequested execution policy.
- Added a canonical endian-stable encoding for one sealed execution result.
- Bound every record to exact caller-supplied request and backend-profile
  identities instead of serializing or reconstructing those semantic bodies.
- Covered diagnostic text and optional retained stream material with a record
  checksum while preserving their existing semantic-identity rules.
- Reconstructed all result shapes through the public invariant-enforcing
  factories, including request-bound cancellation evidence.
- Recomputed evidence identity and required byte-for-byte canonical
  re-encoding after decode.
- Added hard size and field bounds, corruption diagnostics, and distinct
  request/backend authority-mismatch diagnostics.
- Kept execution request, result, backend, cancellation, and identity domains
  unchanged and retained `libpkgexec.so.1`.

## libpkgexec 1.3.0

Dependency-closure migration release.

- Rebuilt the unchanged execution ABI against `libpkgsource 2.0.0`.
- Raised the build-time and pkg-config source floors to 2.0.0 so execution
  adapters cannot reintroduce `libpkgsource.so.1` into a generation-2 process.
- Kept `libpkgexec.so.1`: the public `program`, identity, request, resource,
  control, and result layouts are unchanged.
- Preserved all execution identity domains and backend semantics.

## libpkgexec 1.2.0

Resource-limit capability refinement release.

- Added exact CPU-time, address-space, file-size, open-files, and
  process-count guarantees.
- Made resource-limited requests derive the aggregate guarantee and every
  requested kind.
- Allowed backend profiles to advertise only the limit kinds they can realize
  truthfully.
- Required aggregate and exact limit guarantees to occur together in profiles
  and results.
- Rejected result evidence for resource limits absent from the request.
- Required resource-limit termination evidence to name a requested and
  established kind.
- Preserved existing guarantee ordinals and identities for requests without
  resource limits.
- Kept the backend virtual tables and SONAME unchanged.

## libpkgexec 1.1.0

Controlled execution release.

- Added request-bound monotonic cancellation sources and copyable tokens.
- Added concurrent cancellation observation and blocking token waits.
- Added a controlled backend base without changing the v1.0 backend virtual table.
- Made its ordinary execution path reject cancellation-enabled requests.
- Added exact token admission for controlled execution calls.
- Added not-started and started cancellation result constructors.
- Required matching requested control evidence for cancellation results.
- Kept cancellation timing and control state outside semantic identities.
- Preserved existing disabled-cancellation backend and caller compatibility.

## libpkgexec 1.0.0

Initial native execution-authority release.

- Added sealed build, check, and action-bound lifecycle execution requests.
- Added logical resource layouts and call-scoped concrete resource admission.
- Added closed environment, credential, limit, cancellation, and I/O policy.
- Added backend capability profiles and abstract executor contract.
- Added successful, pre-start failure, and started failure evidence.
- Added complete stream digests and optional retained output material.
- Added domain-separated request, policy, backend, and evidence identities.
- Kept all process syscalls and Linux isolation mechanisms out of the core.
- Added no historical package-manager or fakeroot compatibility.
