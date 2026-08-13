% PKGEXEC_BACKEND(3) libpkgexec | Version 2.1.0


# NAME

pkgexec_backend - executor capability and call-scoped resource contract

# SYNOPSIS

**#include <libpkgexec/backend.h>**

# DESCRIPTION

**backend_capability_profile** seals one backend identity and the guarantees the
backend can establish. **execution_resources::admit()** binds concrete absolute
host paths to exactly the semantic resources retained by one request.

**execution_backend** exposes **capabilities()** and ordinary **execute()** for
requests whose cancellation policy is disabled.

**controlled_execution_backend** adds a token-bearing execution path. Its
ordinary path is final and rejects cancellation-enabled requests. The
controlled path admits only a token bound to the exact request identity before
calling the backend implementation.

The core does not prescribe process-creation, pidfd, signal, or isolation
mechanisms.

A backend must reject an unsupported request before process start. It must not
silently degrade network denial, root-view isolation, resource access,
credential, capture, cancellation, limit, or cleanup guarantees.

Resource-limit support is exact by kind. A profile advertises the aggregate
**resource_limits** guarantee plus only the CPU-time, address-space, file-size,
open-files, and process-count guarantees it can establish truthfully. The
aggregate guarantee and at least one exact kind must occur together.

# SEE ALSO

**libpkgexec**(3), **pkgexec_request**(3), **pkgexec_control**(3),
**pkgexec_result**(3),
**pkgexec_semantics**(7)
