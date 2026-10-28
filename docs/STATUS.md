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
| Extra online | Reimu B, natural power 0; seeds 35964 / 61515 clear in 43,126 / 44,959 actual updates | Two successful samples; eleven full hits retained |
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

The original successful case is `online-acceleration-reimuB-008` under private
`reports/local/extra-online-20261027/`. The original retail terminal and a frozen
independent checker both confirm the entire actual-input route. Every declared
retail projection matches native; native replay matches each observable digest.

| Setting or measurement | Result |
| --- | --- |
| Profile | Base horizon 128, survival horizon 256, lead 16, cap 110 ms, interval 4, candidate target 6 |
| Options | Fresh, pulse-on-failure, laser-aware, anytime, power-aware, damage-aware, acceleration-aware, sleep-limiter |
| Searches / timeouts / publications | 9,590 / 360 / 9,307 |
| Published deadline certificates / ranking yields | 81 / 207 |
| Survival searches / publications | 1,063 / 966 |
| Planned / fallback inputs | 43,122 / 4, all fallback at startup |
| Stale / rejected plans / follower errors | 0 / 0 / 0 |
| Search p50 / p99 / max, including cleanup | 59.54 / 111.27 / 119.00 ms |
| Actual receipt mean / p99 gap / max gap | 57.08 Hz / 27.49 ms / 628.02 ms |
| Whole Wine-tree memory | 342.85 MiB peak, verified 2-GiB/no-swap cap, OOM 0 |

The requested cap includes the whole search group. Cleanup can finish later;
actual overrun/timeout costs remain reported. `--anytime` can retain an already
complete immutable certificate after group cleanup, never unfinished work.
Its 75% ranking reserve makes the candidate target best effort. Full native
verification, lead checks and retail adoption still apply.

**Extra random-seed robustness and stable online 60 FPS remain open.** The
combined shot/seed/profile success is not a single-change attribution. Keep the
failed survival/damage/long-horizon models in [Extra search](EXTRA_SEARCH.md).
The earlier Lunatic component clear uses a separate 64/8/80-ms profile; do not
silently replace its evidence with the Extra settings.

The [2026-10-28 study](EXTRA_SEARCH.md#2026-10-28-stability-and-deadline-study)
retains two new Reimu B failures: seed 24443 at 10,575 updates and seed 4902 at
38,787, spell 62. Both actual routes match independent replay, including each
hit. The latter queue ends at 38,782 after 61 unsuccessful replacement searches
(57 timeouts); five further fallback updates end in the hit. Its verified
2-GiB/1.5-GiB/no-swap limits, peak 393.63 MiB and OOM 0 are retained. The first
trial's interrupted launcher leaves incomplete peak/OOM evidence.

Checked-prefix probes, queue/cleanup diagnostics and child completion timestamps
now expose these failures and enforce the deadline when the parent is
descheduled. A frozen negative control admits a late 40-frame certificate;
the repaired regression rejects it. Removing unused literal-verification scores
reduces child CPU 18.0% in one repeated-origin native workload with identical
routes. All fixed-prefix comparisons and 15 closed native models remain private;
nominal refresh and candidate diversity are optional, with retained failures.

Optional `--render-timing` records actual online update/draw/Present API clocks.
A replay-checked 1,200-update seed-61531 prefix has zero hits/bombs, 1,199
Present samples and no sequence skips/flags. Full Present mean is 40.64 Hz;
steady draw/Present-call means are 0.76/19.18 ms. It fails the pacing gate and
remains incomplete, with no independent window clock. Current accounting is
eleven full hits, two full clears, two prefixes and three earlier zero-frame starts.

The [queue-rescue study](EXTRA_SEARCH.md#2026-10-28-verified-backup-and-shorter-futures)
adds seed **61515**, `online-rescue-002`, under private
`reports/local/extra-control-20261028/`: **44,959 actual updates, NMNB clear**,
complete frozen replay and matching retail projections. The combined profile
adds `--warm-start --rescue-horizon 64 --save-certificates --render-timing`.
Its 9,045 searches publish 8,719 certificates, including 548 verified nominal
backups and 31 shorter futures from 41 short searches. All four fallback inputs
are at startup; stale/rejected plans and follower errors remain zero. Independent
journal checks reconstruct every acknowledged queue and policy transition.
Search p50/p99/max including cleanup is 93.05/124.85/164.13 ms; the verified
2-GiB/1.5-GiB/no-swap group peaks at 447.27 MiB with OOM 0.

The same study retains seed **27718**, a full hit at 24,033 with warm backups
but no short horizon. Its last certificate ends at 24,032. These different-seed
outcomes cannot isolate the rescue policy's effect. Native latency comparisons
retain all fourteen phase episodes, including failures; only six rescue episodes
have the additional complete-trace independent checks documented here.
Full Present cadence for the new clear is **45.83 Hz**, slowest five seconds
22.8 Hz, p99 47.37 ms. Steady draw/Present-call means are 0.94/15.85 ms.
The unchanged pacing gate fails, with no independent window clock; this is
neither a 60-FPS result nor general random-seed robustness.

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

The 2026-10-28 validated build has 47 local CTest contracts with private DATs;
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
