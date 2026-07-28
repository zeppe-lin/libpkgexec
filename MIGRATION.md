# Migration

## From 1.0 to 1.1

Existing backends and callers remain source-compatible for requests whose
cancellation policy is disabled.

A backend that advertises `execution_guarantee::cancellation` must migrate to
`controlled_execution_backend`. It implements separate uncontrolled and
controlled hooks. Cancellation-enabled calls use a
`cancellation_source::for_request()` token; the inherited ordinary execution
path rejects them.

Backends must use `execution_result::cancelled_before_start()` or
`execution_result::cancelled_after_start()` to retain cancellation. The generic
failure factories no longer admit cancellation without matching requested
control evidence.

Whether and when cancellation is requested is operational state. Do not add it
to request or evidence identities.

## Native boundary

`libpkgexec 1.0.0` has no predecessor and no compatibility input format.

Existing build or lifecycle frontends must not translate ambient shell
execution into partially populated execution evidence. They must construct a
complete native request, obtain a conforming backend result, and retain the
result identity through their destination-owned adapter.

The following are not valid native authority:

- a shell command plus exit status;
- inherited process environment;
- a host pathname used as a resource identity;
- a backend name without a capability profile;
- claimed isolation without established-guarantee evidence;
- successful program exit with unverified cleanup;
- historical fakeroot or pkgmk execution receipts.

No importer is included. A future compatibility frontend may observe historical
execution behavior and emit native declarations, but the authoritative library
will not interpret legacy records.
