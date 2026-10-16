# P3-03 — Bounded Windows timing diagnosis (2026-10-16)

[Index](../upstream-port-audit.md). Windows cold/stable acceptance remains open.
This investigation changes neither product logic nor test deadlines.

## Bounded scan recording and current full gates

Windows snapshot `20261016T004104Z-2ca70aab` runs the same password-free
three-consumer/two-probe diagnostic with a90-second Defender performance
recording. Both probes pass3/3,12Qt/0fail/0skip each. The first native LoadDll
takes1010ms, including406ms before the fixture export and604ms in its query;
the second takes605ms with immediate export entry. Maximum GUI heartbeat gaps
are22/20ms. The earlier9-second delay is not reproduced. These passing bounded
probes do not fix or establish cold stability.

The timed recording is requested at00:44:40UTC and finishes at00:46:11UTC. WPR is
confirmed stopped; AMService, antivirus and realtime protection remain enabled
before/after. Its private ETL is2,980,963bytes, SHA256
`7c58133a1dcb32c842b2d053258ec1f3ee34a81c8c05e6d29b931d7a6f50f81f`.
Raw ETL/report stay inside the dedicated guest. Automatic approval review
rejects export of individual scan paths/process information; the safer approved
export contains only numeric aggregates for own project files:183 entries among
TopScans200. Raw duration units are unverified and no timestamp/process/file
correlation is claimed. The first probe starts171ms after the recording request;
coverage of its initial startup was not independently established. This bounded
sample does not establish an antivirus,
scheduling, loader or module-query cause for earlier failures.

The first recorder harness snapshot `20261016T003621Z-b319ac13` fails before
probes because local ExecutionPolicy rejects `-File`. Its source/config restore
and stock rebuild pass. V2 uses the existing stdin/ScriptBlock command transport;
ExecutionPolicy, authorization, security settings and test deadlines are not
changed. V2 restores all six instrumented source files and generated test
configuration, and rebuilds the three stock targets successfully. No temporary
driver remains in the product checkout.

Evidence root below includes `windows-codec-defender-phase-evidence/` and
`windows-codec-defender-phase-v2-evidence/`, with phase logs, source inputs,
restoration receipts, bounded-recording receipt and numeric-only aggregate.
Current full gates are started separately: Linux
`20261016T003859Z-f2ac3bc3`, Windows `20261016T004958Z-53dcb02f`, product2fcc0e1.
[Current Linux whole gate and source proof](p3-06-primitives-2026-10-16.md)
pass for the final product; this does not validate Windows cold stability.
[Current Windows whole gate](p3-03-editor-stability-2026-10-16.md) completes with
one external-editor failure; the three codec/pause consumers pass. This does
not establish a cold-delay fix. Existing no-new-VHD exclusions and eleven slow
suite exclusions are retained.

## Earlier phase probes

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
