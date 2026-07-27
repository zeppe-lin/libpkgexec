# Testing

The model tests cover:

- identity validation and domain separation;
- canonical logical paths and resource slots;
- deterministic resource-layout and environment sealing;
- duplicate resource, mount, variable, and credential rejection;
- typed build, check, and lifecycle purposes;
- exact request sensitivity to program, purpose, interpreter, and policy;
- derived execution guarantees;
- backend capability support;
- successful execution evidence;
- pre-start and started failure taxonomy;
- output capture and cleanup invariants;
- exact call-scoped resource admission;
- the abstract backend contract through a fake backend;
- standalone public-header compilation;
- pkg-config, release, and authority-boundary contracts.

Run the native suite with:

```sh
meson setup --wipe build \
  -Ddefault_library=shared \
  -Dlink_mode=shared
meson compile -C build
meson test -C build --print-errorlogs
```

Static qualification requires a separate build:

```sh
meson setup --wipe build-static \
  -Ddefault_library=static \
  -Dlink_mode=static
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

A production Linux executor is intentionally absent from v1. Tests do not claim
namespace, Landlock, cgroup, network, credential, or process-supervision
coverage.
