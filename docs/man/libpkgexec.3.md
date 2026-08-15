% LIBPKGEXEC(3) libpkgexec | Version 2.2.0


# NAME

libpkgexec - native sealed program-execution authority

# SYNOPSIS

**#include <libpkgexec/libpkgexec.h>**

# DESCRIPTION

**libpkgexec** defines immutable execution requests, call-scoped resource
admission, request-bound cancellation control, backend capability profiles, and
sealed execution evidence for exact **libpkgsource** program values.
It also provides a versioned codec for execution-owned result evidence under
caller-supplied request and backend authorities.

The core performs no process, namespace, mount, cgroup, or filesystem syscalls.
Concrete executors implement **execution_backend** and must refuse requests whose
required guarantees they cannot establish.

# BOUNDARY

The library does not acquire sources, materialize package inputs, build package
artifacts, execute package lifecycle policy, apply filesystem transitions, or
publish installed state.

# ERRORS

Contract failures throw **pkgexec::error**. Branch on **error_code**, not diagnostic
text.

# SEE ALSO

**pkgexec_request**(3), **pkgexec_control**(3), **pkgexec_result**(3),
**pkgexec_result_codec**(3), **pkgexec_backend**(3),
**pkgexec_semantics**(7), **libpkgsource**(3)
