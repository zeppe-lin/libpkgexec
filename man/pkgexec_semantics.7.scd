pkgexec_semantics(7)

# NAME

pkgexec_semantics - native execution authority and identity rules

# AUTHORITY

A *pkgsource::program* remains the program authority. *libpkgexec* seals how that
program is to be executed and what evidence a backend produced. It does not
reinterpret shell material or attach package-build or lifecycle meaning beyond
the typed execution purpose.

# IDENTITY

Request identity includes the exact program digest and language, purpose,
interpreter, root view, logical resources, closed environment, credentials,
limits, cancellation policy, and required guarantees.

Evidence identity includes the request, backend capability profile, start state,
termination, stream digests and byte counts, guarantees established, cleanup,
and typed failure. Diagnostic text and host observations are excluded.

# RESOURCE PATHS

Logical process-visible paths are request policy. Concrete host paths are
call-scoped backend resources and never enter semantic identities.

# FAILURE

Program success does not prove isolation success. Successful evidence requires
all requested guarantees and verified cleanup. Cleanup failure remains failure
even after a zero process exit. A rollback or cleanup attempt is not evidence of
success without verification.

# EXCLUSIONS

The v1 core contains no Linux executor, namespaces, Landlock, cgroups, process
syscalls, source acquisition, package-input materialization, artifact creation,
filesystem application, lifecycle policy, state publication, or historical
compatibility behavior.

# SEE ALSO

*libpkgexec*(3), *pkgexec_request*(3), *pkgexec_result*(3),
*pkgexec_backend*(3)
