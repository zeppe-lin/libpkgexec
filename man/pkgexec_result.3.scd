pkgexec_result(3)

# NAME

pkgexec_result - sealed native execution evidence

# SYNOPSIS

*#include <libpkgexec/result.h>*

# DESCRIPTION

*execution_result* distinguishes a request rejected before process start from a
started process outcome. Successful evidence requires exact interpreter
identity, every requested guarantee, complete requested stream evidence, zero
exit status, and verified cleanup.

Failed evidence retains a typed failure kind, process termination where a
process started, guarantees actually established, stream evidence where
available, and cleanup outcome.

Diagnostic text is retained but excluded from semantic execution identity.
Process IDs, temporary host paths, wall-clock timestamps, and elapsed time are
not execution identity material.

# SEE ALSO

*libpkgexec*(3), *pkgexec_request*(3), *pkgexec_backend*(3),
*pkgexec_semantics*(7)
