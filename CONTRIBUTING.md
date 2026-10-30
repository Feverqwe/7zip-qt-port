# Contributing

Bug reports and focused pull requests are welcome in English or Russian.
For security issues, follow [SECURITY.md](SECURITY.md).

Write public-facing documentation and issue templates in English.

Before reporting a bug, search the existing
[issues](https://github.com/Feverqwe/7zip-qt-port/issues). Include the application
version, OS and architecture, steps to reproduce, expected result and actual
result. For archive problems, include the archive format and a small synthetic
sample when possible. Remove passwords, personal paths and private data from
logs and attachments.

For development, start with the [toolchain and build instructions](docs/development.md)
and [documentation map](docs/README.md). The maintainer's Hyper-V infrastructure
is optional; local builds do not need access to it.

Sevenzip is an independent application based on 7-Zip and other dependencies.
Before changing borrowed functionality, compare its recorded original
implementation and preserve usable code and notices. Project sources follow
local module ownership; dependencies under `third_party/` retain their internal
structure.
[AGENTS.md](AGENTS.md) describes the source-port and repository conventions;
the [port audit](docs/upstream-port-audit.md) records provenance and adaptations.

Keep each pull request focused on an observable problem. Describe the resulting
behavior, relevant upstream source, platforms checked and limitations. Follow
the [focused validation rules](docs/testing.md); test behavior and error handling
rather than source text or private implementation details. Documentation-only
changes need `git diff --check`.

Contributions retain the license of the affected component. Do not remove
third-party notices or assume the root MIT license covers imported code; consult
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
