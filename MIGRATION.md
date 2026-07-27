# Migration

`libpkgexec 0.1.0` has no predecessor and no compatibility input format.

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
