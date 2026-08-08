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
available, and cleanup outcome. Every established guarantee must belong to the
sealed request; a backend profile may advertise additional capabilities, but an
individual result may not claim a different execution policy. Cancellation
before start has no process termination. Cancellation after start retains
cancelled termination. Both require a matching token whose signal was requested.

Limit evidence additionally remains exact by kind. Applying an undeclared limit
changes the execution contract and is not admitted as a stronger result.
Resource-limit failure evidence names the exact requested kind, and both the
aggregate and matching kind-specific guarantees must have been established.
Ordinary nonzero exits, signals, or syscall failures do not become resource-limit
evidence by guess.

Diagnostic text is retained but excluded from semantic execution identity.
Process IDs, temporary host paths, wall-clock timestamps, and elapsed time are
not execution identity material.

# SEE ALSO

*libpkgexec*(3), *pkgexec_request*(3), *pkgexec_control*(3),
*pkgexec_backend*(3), *pkgexec_result_codec*(3),
*pkgexec_semantics*(7)
