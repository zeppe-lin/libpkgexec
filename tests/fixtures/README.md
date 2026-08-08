# Test fixtures

`execution.h` constructs deterministic execution fiction through the real public
`libpkgsource` and `libpkgexec` APIs. It does not replace either authority with
mocks.

Fixtures establish valid authority. Expected semantics and query helpers belong
in `tests/support/`; backend doubles are local to the integration tests that use
them.
