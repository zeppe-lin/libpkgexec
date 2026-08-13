% PKGEXEC_CONTROL(3) libpkgexec | Version 2.1.0


# NAME

pkgexec_control - request-bound call-scoped cancellation authority

# SYNOPSIS

**#include <libpkgexec/control.h>**

# DESCRIPTION

**cancellation_source::for_request()** creates the unique caller-owned source for
one execution request whose cancellation policy is enabled. **token()** returns a
copyable read-only token bound to that exact request identity.

**request_cancellation()** performs a monotonic one-shot transition. The first
transition returns true. Repeated requests are idempotent and return false.
Tokens may query the state or wait for the transition.

The source, token, request time, and observation time are operational control
state. They do not enter execution-request or execution-evidence identity.

# INVARIANTS

A cancellation source cannot be created for a request whose cancellation policy
is disabled. A token for one request cannot control another request.

**controlled_execution_backend** accepts enabled cancellation only through its
token-bearing execution path. Its ordinary execution path is final and rejects
enabled cancellation before backend implementation code is entered.

Cancellation evidence may be sealed only after the matching token reports that
cancellation was requested.

# THREAD SAFETY

Copies of one token may be queried or waited upon concurrently with
**request_cancellation()**. The transition is visible monotonically to every
copy.

# SEE ALSO

**libpkgexec**(3), **pkgexec_backend**(3), **pkgexec_request**(3),
**pkgexec_result**(3), **pkgexec_semantics**(7)
