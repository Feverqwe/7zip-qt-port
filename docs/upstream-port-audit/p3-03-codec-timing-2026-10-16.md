# P3-03 — Bounded Windows timing diagnosis (2026-10-16)

[Index](../upstream-port-audit.md). Windows cold/stable acceptance remains open.
This investigation changes neither product logic nor test deadlines.

The preceding3275293 whole gate executes337/339 successfully and retains two
failures: the15s process_pause CTest cap and external codec-discovery assertions.
Temporary, hash-guarded instrumentation in isolated runner snapshots measures
QProcess startup/completion, immutable module-image copying, native CCodecs
LoadDll, the worker completion signal and the GUI-thread join. Diagnostic test
selection retains original5s assertions, production deadlines and fixture
delays. Logs use verbose QtTest output because `-silent` suppresses qInfo traces.

Snapshot `20261015T202816Z-6cf789d3` records a first diagnostic failure followed
by a passing second execution. On the first, an external-handler child does not
reach the normal completion trace: discovery returns E_ABORT after8502ms.
Another fixture spends9122ms inside native LoadDll and finishes too late for
the original5s assertion. Module copying takes0–2ms and the observed GUI-thread
join takes0ms. On the second execution that fixture's native load takes606ms,
including its deliberate600ms delay. process_pause passes both executions.
These observations locate delayed regions; they do not establish whether the
cause is Windows scheduling, security scanning, loader work or module queries.

Snapshot `20261015T214258Z-3bf4fb3d` adds temporary monotonic timestamps inside
the fixture export and a GUI heartbeat summary. Both bounded three-test probes
pass. The native fixture's first load takes1029ms:404ms before query entry and
624ms in the query, including the intended600ms sleep. Its second load takes
605ms with immediate query entry. Both heartbeat summaries record18ms maximum
measured gaps, with one initial result and one restart result. This successful
run does not reproduce the earlier nine-second native load and cannot prove
that failure's internal phase or responsiveness.

Both snapshots restore every captured source input and rebuild the three stock
targets successfully. The six-file V3 instrumentation patch is retained only
as diagnostic evidence. No backend changes, relaxed assertions, Defender/
security exclusions, WPR configuration, cache flushing or VM configuration
changes were made. Repeating a passing warmed probe is not a stability fix.

Evidence root: `build/review/final-availability-8e9343b-20261014`,
`windows-codec-phase-v2-evidence/` and `windows-codec-phase-v3-evidence/`:
source hashes, exact temporary patches, both per-case logs and restoration/
stock-rebuild receipts. The earlier V1 diagnostic fails safely before probes
on Windows text encoding; its restoration receipt is preserved separately.
