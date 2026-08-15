% PKGEXEC_PROFILE_CODEC(3) libpkgexec | Version 2.2.0


# NAME

pkgexec_profile_codec - durable backend capability-profile authority encoding

# SYNOPSIS

**#include <libpkgexec/profile_codec.h>**

```cpp
backend_capability_profile_encoding encode_backend_capability_profile(
    const backend_capability_profile& profile);

backend_capability_profile decode_backend_capability_profile(
    const backend_capability_profile_encoding& encoding);
```
# DESCRIPTION

The profile codec retains the complete authority owned by one
**backend_capability_profile**: its exact backend identity and canonical guarantee
set. Decoding re-admits that body through **backend_capability_profile::seal**,
verifies the retained profile identity, and requires canonical re-encoding.

The encoding is bounded, versioned at 1, checksummed, and canonical. Unsupported,
corrupt, contradictory, or noncanonical bytes fail closed. The decoder does not
consult or construct a live execution backend.

# BOUNDARY

The profile record owns only backend-profile authority. It does not serialize
execution requests, execution results, source programs, host resources,
controller sessions, or recovery policy. Execution-result evidence remains a
separate qualified record and still requires its caller to supply the exact
request and backend profile bodies.

# SEE ALSO

**pkgexec_result(3)**, **pkgexec_result_codec(3)**, **pkgexec_semantics(7)**
