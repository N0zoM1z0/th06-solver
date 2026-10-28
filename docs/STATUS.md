# Current status

Updated 2026-10-28. Gameplay work focuses on Extra online shadow MPC in the
owned Wine game, following th08-solver. Keep original-runtime, offline/native,
retail replay, recording and online evidence separate. This page owns the current
summary; [Continuation](CONTINUATION.md) owns the operational restart checklist.

## Verified coverage

All clears below are No Miss / No Bomb (NMNB). Native acceptance requires the
source terminal and a complete fresh-process input/digest replay. A prefix or
budget exhaustion stays incomplete.

| Evidence | Verified result | Limit |
| --- | --- | --- |
| Extra online | Reimu B, natural power 0; seeds 35964 / 61515 / 57265 clear in 43,126 / 44,959 / 47,986 updates | Three successful samples; twelve full hits retained |
| Lunatic online components | Two Stage 5 / Reimu B clears and one Stage 6 / Marisa B clear | Five failed Stage 6 controls; no full live story or all-shot coverage |
| Live story handoff | Natural Stage 1 → 2, 20,000-update NMNB prefix | Incomplete story |
| Original-loader replay | Lunatic Reimu A seed 0: 127,737 updates/six clears; Extra Reimu B seed 35964: 43,126 updates/one clear | Both whole traces agree with their fresh native export |
| Replay presentation | One complete audio-off Lunatic story passes both v4 gates | Replay-only result; audio-on recordings and online control do not pass that gate |
| Historical native seed 0 | 96/96 stage/difficulty/shot checkpoints, 16/16 continuous stories | Native proof, including power-128 Extra checkpoints |
| Separate sampled Lunatic campaign | Four seeds: 96/96 standalone cases and 16/16 stories after recovery | First passes 92/96 and 11/16; all failed attempts retained |

[Extra search](EXTRA_SEARCH.md), [live evidence](SHADOW_MPC.md),
[retail replay](RETAIL_REPLAY.md), [native history](NATIVE_HISTORY.md) and
[Lunatic seeds](LUNATIC_RANDOM_SEEDS.md) own the detailed ledgers. Do not treat
different seeds/policies as paired optimization comparisons.

## Current Extra online profile

The original seed-35964 clear uses the [combined profile](EXTRA_SEARCH.md#first-complete-extra-online-nmnb-clear).
The tested warm/rescue continuation adds `--warm-start --rescue-horizon 64`:
base/survival horizons 128/256, lead 16, cap 110 ms, interval 4, six proposals,
fresh/pulse-on-failure/laser/anytime/power/damage/acceleration guidance and
sleep-limiter. `--render-timing --save-certificates` capture actual clocks and
literal published futures; the optional runtime selector adds a window observer.
These options do not change native collision authority or the immutable lead.

| Private case / seed | Actual result | Searches / timeouts / publications | Fallback | Peak / OOM |
| --- | --- | --- | --- | --- |
| `online-warm-001` / 27718, rescue disabled | Hit 24,033, spell 58 | 4,884 / 732 / 4,742 | 4 startup + 1 exhausted | 497.00 MiB / 0 |
| `online-rescue-002` / 61515, Wine 8 | **NMNB clear 44,959** | 9,045 / 1,721 / 8,719 | 4 startup | 447.27 MiB / 0 |
| `online-rescue-003` / 57265, Wine 8 | **NMNB clear 47,986** | 10,066 / 2,818 / 9,432 | 4 startup | 388.54 MiB / 0 |
| `online-runtime-full-008` / 43546, Wine 11/DXVK | Hit 23,036, spell 57 | 4,393 / 1,387 / 4,171 | 4 startup + 2 exhausted | 404.38 MiB / 0 |

All four actual traces match independent retail projection and native digest
replay, including hits. Both new clears pass frozen `--require-complete` replay;
all captured queue/generation/policy transitions pass independent journal checks.
The two clears publish 548 / 1,091 warm backups and 31 / 91 shorter native futures.
All four have verified 2-GiB hard / 1.5-GiB high / no-swap limits. Search cleanup
and actual over-budget costs remain reported; only complete in-budget immutable
checkpoints survive a timeout. See [the full study](EXTRA_SEARCH.md#2026-10-28-verified-backup-and-shorter-futures)
for exact settings, timing distributions, costs and every native control.

The newest [counterexample](EXTRA_SEARCH.md#2026-10-28-wine-11-full-run-counterexample)
is spell 57 / ECL sub 44, boss HP 1,264. Its final 80-update warm plan, from
22,954, ends at 23,034. Nineteen unsuccessful replacements (15 timeouts) follow,
then two fallback inputs and the hit. Three exact checked queues now restore
proposal cursor/recovery as well as the actual prefix and remaining inputs.
Six new 110-ms native phase models retain three hits and three surviving prefixes,
all independently replayed. `diverse-first` loses the late window against its
surviving control, so it is not promoted. These remain timing-dependent models;
all twenty phase episodes and failed attempts are retained in the private ledger.

**Random-seed robustness and stable online 60 FPS remain open.** The two Wine 8
warm/rescue clears and Wine 11 failure are different-seed observations. Full
Present means for the new clears are 45.83 / 40.51 Hz; the Wine 11 full attempt's
actual window mean / slowest five seconds / p99 is 45.42 Hz / 21.8 Hz / 52 ms.
Its steady draw/Present-call means are 1.20/16.02 ms. All unchanged gates fail.

Two native-checked online runtime prefixes (Wine 8 seed 23430 / 1,200 updates;
Wine 11/DXVK seed 14702 / 3,600) have zero misses/bombs and four startup fallback.
Actual window means are 51.96 / 55.95 Hz, peaks 332.81 / 351.63 MiB, OOM 0.
Two separate **pure original-loader replay** controls of the same seed-14702
input prefix run without MPC/follower: window means 58.97 Hz with hardware
bullets and 59.62 Hz with CPU batching, both p99 28 ms and failing gates.
All supplied projection rows match; these are incomplete prefixes. Cache warmth
and shared-host scheduling limit attribution, and batching stays replay-only.

Earlier seed-24443/4902 hits, the seed-61531 rendering prefix, deadline negative
controls and native performance studies remain in [Extra search](EXTRA_SEARCH.md)
and [Performance](PERFORMANCE.md). They are not erased by later successes.
Current actual Extra accounting is **three full clears, twelve full hits, four
prefixes and three zero-frame starts**; the two pure replay diagnostics are separate.

The optional [independent warm horizon](SHADOW_MPC.md#independent-warm-horizon)
now permits a fully verified 64-update nominal backup alongside a normal fresh
search. Covered backups keep the longer acknowledged queue; actual proof length
is recorded separately. Sixteen captured-state native phase models pass fresh
replay, as do their controls. A repeated-origin benchmark preserves all fresh
inputs/digests and reduces native update count, but shows no reliable CPU benefit.
The option remains experimental and defaults to the previous behavior.

## Published replays and recordings

| Input-only replay | Origin | Verified Wine recording |
| --- | --- | --- |
| [`th6_01.rpy`](../replays/th6_01.rpy) | Offline Lunatic Reimu A story, seed 0 | 35:35.744, 503,859,305 bytes, 640×480 H.264/AAC |
| [`th6_02.rpy`](../replays/th6_02.rpy) | Actual online Extra Reimu B route, seed 35964 | 12:04.267, 172,025,731 bytes, 640×480 H.264/AAC |

Both recordings contain original game audio, decode completely and retain all
relative capture timestamps. They are Wine `.rpy` playback recordings; the
separate Sakuya recording is actual live control footage. Media decode and
capture cadence are not game/presentation FPS acceptance.

The audio-off Lunatic pacing sample measures 59.990 Hz / slowest 5 s 58.4 Hz /
p99 18 ms at the actual window, with 468.1-MiB peak and OOM 0. Both audio-on
recordings fail the unchanged pacing gate: Extra reaches 59.972 Hz but its
slowest five seconds are 57.4 Hz. The Extra recording/encoder peaks at
1,119.20 MiB under the owner-authorized 4-GiB/no-swap cap, OOM 0. The
[pacing study](REPLAY_PACING.md) and [pinned replay runtime](REPLAY_RUNTIME.json)
own these separate profiles; [replay provenance](../replays/README.md) owns
file hashes and publication identities.

## Validation and resource rules

The 2026-10-28 validated build has 51 local CTest contracts with private DATs;
CI checks the asset-free subset. [Validation](VALIDATION.md) lists dependencies,
entry profiles, focused tests, full replay checks and export commands.
Use the configured test list rather than old milestone counts in archived notes.

Run one Wine instance at a time, normally with a 2-GiB hard / 1.5-GiB high cap
and swap disabled. The explicit 4-GiB hard / 3-GiB high replay-recording allowance
is recorded separately. Verify actual limits, peaks and OOM counters. Use at
most four solver jobs. Private prototype harnesses are not maintained CLI tools.

Keep the historical seed-0 matrix and every failed retry. Do not rerun full
matrices/stories or schedule other difficulties for routine optimization.
[Performance](PERFORMANCE.md) records native timing improvements and optional
retention tradeoffs; those are not online speedup or robustness guarantees.

## Next work and evidence

1. Improve Extra route choices on selected captured hard windows, then bounded
   actual Wine samples. Retain fixed-prefix and latency-model counterexamples.
2. Measure online queue risk and presentation together; a replay-only FPS pass
   cannot close online pacing. Preserve all loading/transition gaps.
3. Extend checked-prefix diagnostics to repeatable policy episodes with the
   original remaining queue before broadening Lunatic live coverage or attempting
   a complete online story.

[Continuation](CONTINUATION.md) gives acceptance checks and source entry points.
The open tracker is [issue #1](https://github.com/N0zoM1z0/th06-solver/issues/1).
Private evidence roots include `reports/local/extra-control-20261028/`,
`reports/local/extra-stability-20261028/`,
`reports/local/extra-online-20261027/`,
`extra-search-20261027/`, `shadow-extra-20261027/`, `shadow-20261025/`,
`pacing-20261026/`, `extra-video-20261027/` and `replay-video-20261027/`.
[Native history](NATIVE_HISTORY.md) and [online history](ONLINE_HISTORY.md)
preserve earlier tables, costs, bootstrap corrections and handoff notes.
