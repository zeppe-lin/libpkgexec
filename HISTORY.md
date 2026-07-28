# History

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
