# libpkgexec

`libpkgexec` is the native Zeppe-Lin program-execution authority.

It seals how one exact `libpkgsource` program is to be executed and retains
what an execution backend actually established and observed. The core library
contains no process syscalls and no Linux-specific executor.

The public contract separates three things that must not be collapsed:

1. an immutable semantic execution request;
2. call-scoped concrete resources supplied to a backend;
3. immutable execution evidence returned by that backend.

A zero process exit is not sufficient for success. Successful evidence also
requires the exact interpreter, every requested guarantee, complete requested
stream captures, and verified cleanup.

## Scope

The v1 core owns:

- build, check, and action-bound lifecycle execution purposes;
- logical read-only and writable resource layouts;
- a closed process environment;
- exact numeric credentials;
- resource limits and cancellation policy;
- backend capability profiles;
- started and not-started failure evidence;
- output content identities and optional retained material;
- domain-separated request and evidence identities.

It does not own:

- source acquisition or extraction;
- package-input materialization;
- build-result or artifact construction;
- filesystem application;
- lifecycle policy;
- installed-state publication;
- Linux namespaces, Landlock, cgroups, or process supervision;
- CRUX, pkgmk, fakeroot, or pkgman compatibility.

## Build

```sh
meson setup build \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build
meson test -C build --print-errorlogs
```

Shared and static builds are separate. `default_library` and `link_mode` must
select the same mode.

## Dependency

`libpkgexec 1.0.0` requires `libpkgsource >= 1.1.0` and OpenSSL libcrypto.

## License

GPL-3.0-or-later.
