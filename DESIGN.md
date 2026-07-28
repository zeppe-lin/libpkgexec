# libpkgexec design

## Authority

`pkgsource::program` remains the authority for program language and exact
material. `libpkgexec` does not parse or reinterpret that material. It adds the
execution contract and the evidence domain around it.

The core authority is:

```text
exact source-owned program
+ typed purpose
+ interpreter identity
+ root-view identity
+ logical resource layout
+ closed environment
+ credentials, limits, cancellation
+ required guarantees
= sealed execution request
```

An execution backend combines that request with call-scoped host resources and
returns sealed evidence.

## Request domains

Execution purposes are build, check, or lifecycle. Lifecycle requests are bound
to exactly one `pkgsource::lifecycle_action`.

Logical resources use typed roles. Source, build-input, and check-input trees
are read-only. Workspaces, package-output roots, and private temporary roots are
writable. Managed target roots may be read-only or writable according to the
request.

Logical process paths are semantic policy. Host paths are not. Host paths occur
only in `execution_resources` and never enter request or evidence identities.

## Environment

The environment is closed. Locale, timezone, home, `PATH`, temporary directory,
parallelism, umask, network access, stdin, stdout, stderr, and
`SOURCE_DATE_EPOCH` are explicit. Additional variables are admitted by exact
name and value. The backend must not inherit caller variables silently.

## Cancellation control

Cancellation policy is semantic request authority. It states whether
cancellation is disabled or graceful-then-forced and retains the exact grace
period. The event that asks one live execution to cancel is operational state.
It must not change request identity.

A `cancellation_source` is created for one cancellation-enabled request and
owns a monotonic one-shot transition. Its copyable token retains the exact
request identity, permits lock-free state queries, and permits blocking waits.
No global flag, ambient signal handler, controller pathname, or process
identifier is cancellation authority.

`controlled_execution_backend` preserves the original backend ABI while adding
an admitted control path. Its final ordinary path accepts only disabled
cancellation. Its token-bearing path validates that the supplied token belongs
to the exact request before backend implementation code runs.

A cancellation result requires the matching token to have transitioned.
Cancellation before process start is retained as failed, not-started evidence
without process termination. Cancellation after process start retains cancelled
termination and the normal started-execution cleanup and capture obligations.

## Capabilities and guarantees

A backend capability profile says which guarantees a backend can establish.
A request retains the guarantees it requires. An unsupported request must be
refused before process start.

Evidence retains the guarantees actually established. Success requires all
requested guarantees. A process may fail after valid isolation was established;
that distinction remains visible.

## Result domains

A result distinguishes not-started from started execution. Pre-start failures
include unsupported requests, resource admission, interpreter, isolation setup,
and process start. Started failures include nonzero exit, signal termination,
resource limits, cancellation, log capture, and cleanup.

Cleanup failure is failure even after exit status zero. Diagnostic text is
retained but excluded from semantic identity.

## Backend boundary

`execution_backend` is abstract. Its original two-argument surface remains the
boundary for requests with disabled cancellation. `controlled_execution_backend`
adds request-bound cancellation without changing that existing virtual table.

The core does not choose process creation, mount realization, namespace,
Landlock, cgroup, signal, pidfd, or log-capture mechanisms.

A future `libpkgexec-linux` may implement those mechanisms. It must refuse a
request if a requested guarantee cannot be established; silent degradation is
not conforming behavior.

## Dependency direction

```text
libpkgsource
    ↓
libpkgexec
    ↓
future destination-owned adapters
    ├── libpkgbuild-exec
    └── libpkgapply lifecycle integration
```

`libpkgexec` does not depend on build, apply, state, transaction, or controller
libraries.
