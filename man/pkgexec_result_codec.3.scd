pkgexec_result_codec(3)

# NAME

pkgexec_result_codec - durable native execution-result evidence encoding

# SYNOPSIS

*#include <libpkgexec/result_codec.h>*

*execution_result_encoding encode_execution_result(*
    *const execution_result& result);*

*execution_result decode_execution_result(*
    *const execution_result_encoding& encoding,*
    *execution_request request,*
    *backend_capability_profile backend);*

# DESCRIPTION

The execution-result codec retains the observation fields owned by
*libpkgexec*: process-start state, interpreter observation, termination,
stream evidence, established guarantees, cleanup, typed failure, diagnostic
text, and the sealed evidence identity.

The semantic execution request and backend capability profile are not
serialized or reconstructed from identities. Their exact identities are
stored in the record, and the caller must supply both complete authorities to
*decode_execution_result*.

Decoding verifies the record checksum before interpreting authority fields,
requires the supplied request and backend profile identities, reconstructs the
result through the ordinary invariant-enforcing factories, recomputes the
execution-evidence identity, and requires canonical re-encoding.

Cancellation evidence is reconstructed with a fresh request-bound
*cancellation_source* whose signal is transitioned before the cancellation
factory is called. The operational cancellation state is not retained as
semantic identity.

Diagnostic text remains outside execution-evidence identity, but it is covered
by the durable record checksum. Altering a diagnostic therefore corrupts the
record even though it does not change semantic result identity.

# LIMITS

One encoding is limited by *maximum_execution_result_encoding_size*. Diagnostic
and retained stream material fields have bounded lengths. Oversized, truncated,
noncanonical, checksum-invalid, or semantically impossible encodings are
rejected with *error_code::corrupt_encoding*.

Supplying another request or backend profile is rejected with
*error_code::authority_mismatch*.

# BOUNDARY

The codec does not serialize source programs, execution requests, backend
profiles, host resource paths, cancellation state, build results, check
results, lifecycle results, or controller sessions. It is a subordinate
evidence codec, not an execution-session archive or recovery policy.

# SEE ALSO

*libpkgexec*(3), *pkgexec_request*(3), *pkgexec_result*(3),
*pkgexec_control*(3), *pkgexec_semantics*(7)
