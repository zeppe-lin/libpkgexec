# Contributing

Changes must preserve the authority boundaries documented in `DESIGN.md` and
`MAINTAINING.md`.

Use C++17, GPL-3.0-or-later SPDX headers, strict compiler warnings, and focused
model tests. Commit subjects should identify the authority boundary being
changed.

Do not add process execution, Linux isolation, package-build orchestration,
lifecycle policy, filesystem application, installed-state publication, or
historical compatibility to the core library.
