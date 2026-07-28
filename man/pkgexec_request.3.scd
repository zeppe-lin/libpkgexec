pkgexec_request(3)

# NAME

pkgexec_request - sealed native execution request

# SYNOPSIS

*#include <libpkgexec/request.h>*

# DESCRIPTION

*execution_request::seal()* binds exact program bytes, typed purpose,
interpreter identity, root-view identity, logical resource layout, closed
environment, numeric credentials, resource limits, cancellation policy, and all
required execution guarantees.

Build, check, and lifecycle purposes are distinct. Lifecycle purpose is bound to
one exact *pkgsource::lifecycle_action*.

Concrete host paths are not request authority. They are admitted separately by
*execution_resources* at backend call time. Cancellation policy is request
identity; the live cancellation signal is admitted separately through
*pkgexec_control*(3).

# INVARIANTS

Resource slots and logical mount points are unique. Source and package-input
resources are read-only. Workspaces, package-output roots, and private temporary
roots are writable. Environment-variable order and resource declaration order
are normalized where order is not semantic.

# SEE ALSO

*libpkgexec*(3), *pkgexec_control*(3), *pkgexec_backend*(3),
*pkgexec_semantics*(7)
