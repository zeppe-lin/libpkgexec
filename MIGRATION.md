# libpkgexec migration

## From 2.1.1 to 2.2.0

`resource_role::package_tree` is the neutral singleton role for an immutable
package that is the subject of execution. It is not a named build/check input.
Existing role numeric values are unchanged and the SONAME remains 2. Rebuild
consumers that need the new role; old execution evidence remains bound to its
unchanged request authority.


## From 2.1 to 2.1.1

The public ABI remains `libpkgexec.so.2`, but the provider closure changes to
`libpkgsource >= 4.0.0, < 5.0.0`. Rebuild execution consumers against 2.1.1 and
do not mix the source-3-linked 2.1.0 DSO with source-4 authority. Execution and
backend-profile identity domains are unchanged.

## From 1.4 to 2.0

`libpkgexec 2.0.0` advances the shared ABI to `libpkgexec.so.2`. Rebuild every
direct execution consumer; do not load `libpkgexec.so.1` and `.so.2` into one
native toolchain closure. The source dependency is now explicitly bounded to
`libpkgsource >= 3.0.1, < 4.0.0` and shared qualification requires
`libpkgsource.so.3`.

The semantic execution model and durable result format are unchanged. The ABI
change establishes a reviewed compiler-independent public export set instead of
preserving private constructors and implementation symbols accidentally exported
by the v1 DSO. Public polymorphic boundaries now have DSO-anchored destructors.

Ordinary factory admission is stricter: unsupported raw enum values for
execution purposes, resources, environment policy, guarantees, termination,
failure kinds, and cleanup are rejected rather than entering identities or
evidence as unknown vocabulary. Valid v1.4 callers require no semantic rewrite.

## From 1.3 to 1.4

The execution request, backend, result, cancellation, and identity APIs remain
compatible. `libpkgexec.so.1` is unchanged.

Callers that need durable subordinate execution evidence may now use
`encode_execution_result()` and `decode_execution_result()`. The encoding is
not a self-contained execution session: decoding requires the exact original
`execution_request` and `backend_capability_profile`. Do not replace those
values with objects inferred from the identities stored in the record.

Treat `corrupt_encoding` as record-integrity or semantic-shape failure. Treat
`authority_mismatch` as selection of the wrong request or backend authority.
Neither error authorizes retrying execution or accepting partial evidence.

The codec preserves diagnostic text and retained stream material, but these
remain outside execution-evidence identity. The record checksum, not the
semantic identity, detects changes to those fields.

## From 1.1 to 1.2

Resource-limit support is now profiled by exact kind. A request with any limit
still requires `execution_guarantee::resource_limits` and additionally requires
one guarantee for every populated field:

- `cpu_time_limit`;
- `address_space_limit`;
- `file_size_limit`;
- `open_files_limit`;
- `process_count_limit`.

Backends must advertise the aggregate guarantee plus only the exact kinds they
can realize. The aggregate guarantee and at least one exact kind must occur
together; either side alone is rejected as an invalid capability profile.
Execution evidence follows the same shape and may claim only requested kinds.

The identities of requests containing resource limits intentionally change
because their required-guarantee set is now exact. Existing guarantee enum
values were preserved and the new values were appended, so requests and
profiles that contain no new limit guarantees retain their previous identities.
The public class layouts, backend virtual tables, and SONAME remain unchanged.

`resource_limit_exceeded` evidence must identify a limit present in the sealed
request and must establish both the aggregate and matching kind-specific
guarantees. Do not infer limit termination from an ordinary nonzero exit, an
allocation failure, or an unrelated signal.

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

No importer is included. Historical execution behavior remains outside the
native authority boundary.
