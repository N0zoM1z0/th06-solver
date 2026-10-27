# Continuation: Lunatic online shadow MPC

2026-10-27 current owner scope: Extra online shadow MPC with Reimu A, normal
power-0 retail entry and captured seeds. One incomplete prefix passes; eight
completed full attempts hit. Complete Extra online NMNB and live 60 FPS remain
open. The latest seed 5594 hits at update 22,062 on spell 57 with 946 boss HP;
all retail projections and independent native digests agree including the hit.

Current optional refinements include laser geometry/orbits, immutable anytime
certificates, a ranking-time reserve, native power pickups, survival-only longer
lookahead, boss-damage scoring and finite-acceleration bullet forecasts. Forty
local CTest contracts pass.
Promoted native models improve early power and one survival window, but retain
failed seeds. Global 256-tail spell-58 models exhaust the 110-ms budget and fail;
keep the 128 base horizon. Damage-aware 128-tail models pass two captured windows
and fail a third, so ranking is still experimental.

Continue with earlier candidate-route choices before the spell-57/58 bottom-edge
and spell-62 right-edge traps. Acceleration model controls and guided routes all
pass two captured windows, with mixed completion times; keep the flag optional. A native full future, fewer timeouts or a completed
spell prefix is not a complete retail clear. Keep actual search/queue timings,
all retries and fresh full-input validation. See [focused evidence](EXTRA_SEARCH.md),
[status](STATUS.md) and [the full live ledger](SHADOW_MPC.md). Private evidence
roots are `reports/local/shadow-extra-20261027/`,
`reports/local/extra-search-20261027/` and
`reports/local/extra-online-20261027/`. The Lunatic ledger below is historical.

Earlier prerequisite, owner clarification 2026-10-26: stabilize
[actual Wine `.rpy` replay pacing](REPLAY_PACING.md), using an independently
verified native export and no MPC/native follower during playback. The online
roadmap below remains open; playback performance does not close online
frame-pacing or policy-robustness tasks.

The pure-playback prerequisite now passes one complete Lunatic Reimu A seed-0
story: 127,737 native-matching NMNB updates and six natural clears. Independent
window output is 59.990 Hz / slowest 5 s 58.4 Hz / p99 18 ms; the API gate also
passes. Peak is 468.1 MiB, OOM 0. The retained complete-story CPU comparison falls
26.7%. The source profile combines bounded CPU/diffuse quad batches, mapped
journals, scoped coarse sleeps, eight software-raster workers and an opt-in
[DXVK EndScene submission patch](DXVK_REPLAY_BUILD.md).

Earlier failures remain important: the official-DXVK FIFO profile completes the
story but fails at 56.4 Hz in dense Stage 4; coarse sleep alone repeats that
failure. Across 1,201 target frames, geometry bytes and sampled D3D states match
selected-stage drawing. Mapped collection removes per-frame Windows writes and
CSV formatting; early submission overlaps graphics with the original limiter
wait. The final natural story crosses that old window at 59.4 Hz / p99 18 ms.
This is a combined-profile result, not a universal single-change attribution.

All handoff gaps remain in the gate, including a 145-ms maximum window interval.
Reducing that loading spike remains replay work. The online roadmap below stays
open: this run contains no MPC/native follower and does not certify online FPS,
controller deadlines or all-seed robustness. See [REPLAY_PACING.md](REPLAY_PACING.md),
[REPLAY_RUNTIME.json](REPLAY_RUNTIME.json) and private evidence under
`reports/local/pacing-20261026/`. All 50 attempts, including failures, withdrawn
controls and accelerated diagnostics, remain in the local ledger.

Handoff written 2026-10-26, based on implementation
[`3880700`](https://github.com/N0zoM1z0/th06-solver/commit/388070073e0e0c4cae1bf0af66e6e567916a4841)
and experiments from 2026-10-25. This is the restart point for unfinished work;
[SHADOW_MPC.md](SHADOW_MPC.md) specifies the existing controller and run commands.
Track progress and experiment updates in
[issue #1](https://github.com/N0zoM1z0/th06-solver/issues/1).

**Online shadow MPC does not yet sustain stable 60 FPS.** Three live component
clears establish gameplay progress, not smooth real-time performance. Hard-seed
robustness, broader live stage/shot coverage and complete live story control also
remain open. The Sakuya recording was an intermediate deliverable; subsequent
work added collision-phase proposals, long-plan transport and a Stage 6 clear.

## What has been completed

| Layer | Verified result | Scope of the evidence |
|---|---|---|
| Native source runtime | Seed-0 standalone matrix 96/96 and continuous stories 16/16, complete fresh NMNB replay | Historical native profile, all difficulties; not current online coverage |
| Native Lunatic random-seed campaign | 96/96 stages and 16/16 stories after recovery; first passes 92/96 and 11/16 | Four sampled seeds, with all failed attempts/retries retained |
| Native search optimization | Unchanged-input replays about 1.5–2× faster; optional `--reuse 32` reduces speculative updates 71.75% and planner CPU 64.83% in 20 paired cases | First-pass success falls 18/20 to 16/20; not an online speedup claim |
| Offline route → retail replay | Complete Lunatic Reimu A story, 127,737 updates, six natural clears, zero misses/bombs | Actual Wine playback of the tracked [input-only replay](../replays/th6_01.rpy); decisions were made offline |
| Pure replay frame pacing | Complete 127,737-update Lunatic story, 59.990 Hz / slowest 5 s 58.4 Hz / window p99 18 ms | Mapped collection, source CPU batches, coarse sleep, eight-worker WoW64/lavapipe and opt-in DXVK early submission; online pacing remains separate |
| Live Wine shadow MPC | Two Stage 5 / Reimu B clears and one Stage 6 / Marisa B clear, all NMNB with full independent native replay of actual inputs | Three finite component successes; five actual Stage 6 failures remain |
| Live story lifecycle | Natural Stage 1 → 2 handoff; 20,000-update NMNB prefix with fresh native replay | One handoff, not a six-stage live story |
| Sakuya video | Actual live Stage 5, game audio, 640×480 H.264/AAC, 22,602 captured frames, about 377 seconds | Zero duplicate capture timestamps; capture cadence does not certify game FPS |

Historical native results and offline retail playback remain separate from online
acceptance. See [STATUS.md](STATUS.md), [LUNATIC_RANDOM_SEEDS.md](LUNATIC_RANDOM_SEEDS.md),
[PERFORMANCE.md](PERFORMANCE.md) and [RETAIL_REPLAY.md](RETAIL_REPLAY.md).

The online implementation now includes:

- A common retail/native start, actual action-dependent source RNG and an ordered
  per-update receipt journal. Native follows actual inputs, never speculative ones.
- Fork-owned speculative worlds, immutable queued input leads, exact internal
  native input/digest checks, and retail rejection of stale or changed-lead plans.
- Whole-search deadlines covering proposals, native verification, digest replay,
  IPC and descendant cleanup. Timeout retains the prior verified queue.
- Version-2 mapped plans of up to 600 inputs and atomic `SOCK_SEQPACKET` child
  certificates. Larger verified futures can actually be queued in full.
- Optional `--fresh` / `--proposals`, rotating `--pulse` recovery, and conditional
  collision geometry with independent native verification. Defaults remain
  tail 128 / lead 16 / cap 110 ms / interval 16 / minimum one candidate.
- Deferred hashes on later candidates, independent digest-producing winner replay
  and cached follower state between receipts. Offline defaults are preserved.
- One owned Wine instance, frozen controller/DLL/runner identities, archive/config
  hashes, and a shared 2-GiB hard / 1.5-GiB high / no-swap cgroup for the whole run.
- Twenty-three focused contracts, complete fresh replay of all three live clear
  routes, and [green baseline CI](https://github.com/N0zoM1z0/th06-solver/actions/runs/40221693476).
  CI uses asset-free tests; the local focused suite additionally uses private DATs.

## Actual online trial ledger

All cases below are Lunatic. Stage 5 uses Reimu B; Stage 6 uses Marisa B. Seeds
were captured from actual retail initialization. Case names identify directories
under the private `reports/local/shadow-20261025/` evidence root.

| Case | Seed | Profile or purpose | Actual updates | Outcome |
|---|---:|---|---:|---|
| `probe001` | 24248 | Initial controller smoke | 1,200 | Budget-limited prefix |
| `stage5-001` | 51280 | 128 tail / 16 lead / 110 ms / interval 16 | 22,360 | Complete NMNB |
| `capture-probe001` | 48834 | Initial capture; rounded duplicate timestamps retained | 600 | Budget-limited prefix |
| `capture-probe002` | 9260 | Corrected capture timestamp smoke | 600 | Budget-limited prefix |
| `stage5-video001` | 42686 | Initial controller profile plus recording | 22,317 | Complete NMNB |
| `stage6-deferred001` | 47489 | Initial controller profile | 14,230 | Hit; incomplete |
| `story-handoff001` | 33585 | Natural Reimu B story start, initial profile | 20,000 | Stage 2 reached; incomplete story |
| `stage6-h240-001` | 32558 | 240 tail / 16 lead / 110 ms | 5,200 | Hit; incomplete |
| `stage6-pulse001` | 56175 | 240 tail plus rotating native motions | 3,617 | Hit; incomplete |
| `capture-snapshot003` | 42245 | Frozen Python runner/capture smoke | 600 | Budget-limited prefix |
| `stage6-recorded001` | 57766 | Recorded geometry, original one-endpoint-per-refinement policy | 5,174 | Hit; incomplete |
| `stage6-h512-buffer001` | 18751 | 512 tail / 40 lead / 480 ms / interval 32 | 5,002 | Hit; incomplete |
| `stage6-fresh-p3-001` | 10347 | 64 tail / 8 lead / 80 ms / interval 4 / fresh / minimum 3 candidates | 28,254 | Complete NMNB |

Totals: **3 complete components, 5 failed Stage 6 components, 5 incomplete
prefixes**. None is a complete live story. Different seeds, policies and code
revisions make these an attempt ledger, not an A/B success-rate comparison.
Every trial has verified 2-GiB/no-swap limits and zero OOM/OOM-kill events.
The largest group peak is 821,743,616 bytes during recording/muxing.

The Stage 6 clear has 7,050 searches, 63 timeouts, two completed failures and
6,985 publications. Search median/p99/max is 29.07/77.70/82.87 ms including cleanup.
Only the first four inputs use fallback; all subsequent inputs use verified plans.
There are no stale publications or retail rejections. Group peak is
354,058,240 bytes. Controller/child CPU is 24.31/201.69 seconds across 483.43
seconds of controller wall time. Completed work is 26,255 branches / 1,614,007
updates, excluding unknown interrupted work. Interval four is a minimum launch
spacing, not a guarantee of one completed decision every four game updates.

## Stable 60 FPS is still unresolved

The owner reports that online shadow MPC is not yet stable at 60 FPS. Existing
retail update timestamps also contain substantial jitter:

| Actual live case | Retail receipt interval p99 | Maximum interval |
|---|---:|---:|
| Stage 5, seed 51280 | 31.00 ms | 403.65 ms |
| Stage 5 recording, seed 42686 | 25.49 ms | 435.99 ms |
| Stage 6 clear, seed 10347 | 28.87 ms | 477.50 ms |

These are full-run `receipt_gap_*` metrics from retail `QueryPerformanceCounter`
timestamps at the post-gameplay callback, not controller journal-poll delays.
The current summaries do not separate startup, dialogue, transitions and active
combat; the worst gaps cannot yet be attributed to searches or rendering.
Render/present timing is not independently instrumented. The lack of an explicit
whole-frame controller wait does not prevent CPU contention or scheduling stalls.

Keep three measurements separate: game update/present cadence, search latency,
and capture timestamps. A 29-ms median search can be hidden behind a queued lead,
but this alone establishes no frame-rate guarantee. Likewise, a smooth capture
stream can sample repeated game images. The video's 17-ms p99 sample spacing,
full decode, successful NMNB terminal and average route throughput are insufficient
to claim stable 60-FPS online play. The target is approximately 16.67 ms per
displayed frame; search need not complete every displayed frame.

## Open work, in recommended order

### P0: Attribute frame pacing and establish a 60-FPS gate

- Add update and render/present measurements with a common monotonic time base.
  Keep full-run statistics and separately labeled startup, dialogue, handoff and
  active-combat windows. Report rolling FPS, interval p50/p95/p99/max and counts
  above 20/33.3/50 ms; retain all gaps rather than hiding initialization costs.
- Compare the same retail route and configuration with MPC disabled, follower
  only, search enabled, and capture enabled. Control input-dependent RNG and path:
  use a retained literal route or another explicit paired fixture. New random
  seeds are not an adequate frame-pacing control.
- Correlate spikes with search launches/completion, forks, page faults, journal
  writes, CPU scheduling, software rendering/Xvfb and recording. CPU contention,
  fork/COW and rendering cost are hypotheses, not established root causes.
- Change scheduling, rollout volume or rendering/capture settings only after
  attribution. Preserve source clocks and actual retail gameplay semantics.

Acceptance: a repeatable hard-stage test reports both game and presentation
cadence, a declared hardware/display configuration and explicit jitter tolerance
around the 60-FPS target. It must demonstrate sustained combat cadence without
recurring search-correlated stalls, while preserving complete NMNB playback,
native follow/replay correctness, deadlines and memory limits. Until measured,
stable 60 FPS remains unchecked even when the stage clears.

### P0: Make failed windows repeatable and measure queue risk

The strongest retained case is Stage 6 / seed 47489. Its last accepted plan
originates at 14,080; searches time out through 14,208, the queue expires at
14,224, and retail hits at 14,230. At origin 14,112 plus the actual 16-input lead,
all 50 unrestricted diagnostic futures reach at most 101 safe tail updates.
A larger deadline alone cannot certify the requested 128. At origin 13,952 a
240-update future is found in about 72 ms; this does not prove that adopting it
would clear the stage. Full Native/retail projected rows agree, including the hit.

- Promote the useful private checked-prefix probes into a maintained tool. Take
  a native action tape, actual origin/lead, candidate settings and binary identity;
  preserve input/digest validation and report exactly what world was tested.
- Separate fixed-prefix proposal comparison from complete policy episodes.
  Compare both fixed execution lag and explicitly labeled measured-wall-time lag.
  Current private timing models consume `max(1, ceil(search_ms * 60 / 1000))`
  actual updates per decision, so host load can change their policy trajectories.
- Add per-decision queue remaining, certificate age, consumed lead, finish/adoption
  lag, verification/cleanup time and fallback reason. Current phase timeout
  counters and `decisions.jsonl` are useful but do not provide this full timeline.
- Preserve failed attempts, incomplete futures and interrupted-work exclusions.
  Current branch/update totals cover completed searches only.

Acceptance: one command reconstructs a retained failed window and validates its
exact native prefix; the report distinguishes timeout, completed no-certificate,
stale rejection, stage reset and queue exhaustion. Synthetic blocked descendants
still get killed/reaped without changing the parent world.

### P1: Improve candidate quality and decision timing before the trap

The 512-tail control still fails: its final two searches finish without a safe
certificate in 246/232 ms under a 480-ms cap. The fresh short profile clears seed
10347 but also fails seed 47489 in a separate native timing model. That model
fails under initial / fresh-three / fresh-six profiles at 14,505 / 6,788 / 12,181
updates. These are native models, not additional Wine runs or a paired proof that
one actual retail policy is better.

Investigate fresh-versus-nominal scheduling, horizon/lead tradeoffs, route
diversity and earlier detection of shrinking escape space using the maintained
failed windows. Compare one change at a time, then bounded complete episodes.
CPU spent evaluating more candidates must be balanced against game pacing and
remaining lead. Current `--proposals` is a minimum evaluated-candidate count,
not a requested number of successful futures or a hard upper bound.

Acceptance: improvement on preselected identical native prefixes and episodes,
then actual bounded Wine components; include every failure/retry, full winner
verification, queue depletion, timing distributions and total CPU. Do not replace
the default solely because one new seed clears. A depleted queue's held input
is fallback, never a certified rescue.

### P1: Validate and profile conditional geometry

`--recorded` is optional and off by default. The first live run adopts 75 native-
verified geometry proposals but still hits at 5,174. The current four-endpoint
portfolio also cannot rescue the late 47489 windows within 110 ms. The source
model is conditional on its forecast's actions/RNG; unioning failed forecasts
can overconstrain proposals, and coarse controls can miss a narrow route.

Check movement/collision phase timing, distant ungrazed bullets, clockwise laser
rotation, time-stop/speed context, aiming dependence, proposal control spans and
refinement diversity. Use genuine native counterexamples to distinguish model
error, poor scoring and candidate-family limits before expanding the beam.
Profile capture, index construction, beam expansion, native verification and
digest replay separately. Existing timeout counters identify a phase but do not
provide complete per-phase cost attribution for all interrupted work.

Acceptance: passive hooks preserve golden native digests; model-selected paths
always need independent native execution plus digest replay. Keep 240-frame /
2,048-box / 512-laser bounds, five raw tape slots, a 32-MiB combined spatial-index
entry cap and explicit capacity failures. Proposal-only SSE2 arithmetic cannot
replace source x87 collision/RNG authority.

### P2: Expand Lunatic live coverage, then complete natural stories

Current live clears cover only Stage 5 Reimu B and Stage 6 Marisa B. They do not
cover every main-game stage/shot, all hard seeds or a complete live story.
Stabilize the selected hard cases and pacing first. Then predeclare a bounded
Lunatic stage/shot/seed sample; the longer-term scope is six main-game stages,
four shots and natural six-stage stories for the four shots. Do not rerun the
historical 96-case/all-difficulty matrix for routine optimization.

The current runner captures a retail seed but has no `--seed` option. Repeating
the command does not reproduce a case. A controlled retail common-start fixture
is future work and must preserve the source's initialization/RNG sequence; until
then, paired native diagnostics and independent random retail trials must be
labeled separately. Never transplant a pointer-rich world or reseed a handoff.

Acceptance: standalone cases reach the real terminal with zero misses/bombs and
full fresh native replay of actual inputs. A story must start naturally at
Stage 1 / power 0, carry source resources/rank/RNG through all handoffs, and reach
Stage 6's natural terminal with six clears. Keep stable frame pacing, deadline,
queue, CPU and whole-group memory evidence alongside gameplay results.

## Resume commands and evidence

Read [AGENTS.md](../AGENTS.md), this handoff and [SHADOW_MPC.md](SHADOW_MPC.md)
before changing behavior. Run only the checks relevant to a new change. For a
native/bridge change, the existing focused local gate is:

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release -DTH06_WINE_REPLAY=ON \
  -DTH06_DATA_DIR=/path/to/private-th06-data
cmake --build build -j 2
ctest --test-dir build --output-on-failure \
  -E 'real_data_contracts|story_transition_contracts|story_terminal_contracts|extra_bootstrap_contracts'
build/th06_replay_check --data /path/to/private-th06-data \
  --tape reports/local/shadow-20261025/stage6-fresh-p3-001/actual.actions \
  --require-complete
```

The existing local suite has 23 focused tests. The replay command requires that
private tape; a fresh clone alone cannot reproduce the historical route.
Wine setup, runner profiles and capture prerequisites are documented in
[SHADOW_MPC.md](SHADOW_MPC.md) and [RETAIL_REPLAY.md](RETAIL_REPLAY.md).
The current live commands start new random cases; they are not seed selectors.

| Private artifact | Purpose |
|---|---|
| `reports/local/shadow-20261025/acceptance-final.json` | All 13 actual trials, identities, resource receipts, checks, video hash and separate native timing models |
| `<case>/start.txt`, `worker.json`, `bin/` | Actual start, parameters and frozen executable/Python identities |
| `<case>/receipts.bin`, `actual.actions` | Retail update/input/projection/timestamp journal and checked native input/digest tape |
| `<case>/decisions.jsonl`, `controller.json` | Origins, catch-up, certificates, timeouts, publications, CPU and queue accounting |
| `<case>/resource-receipt.json`, `outcome.json` | Whole-group limits/peak/OOM and actual retail terminal/failure |
| Complete cases' `native-final-check.json` | Full new-process replay of the actual clear route |
| `profiles/stage6-failure.json`, `recorded-counterexample-portfolio4.json` | Retained fixed-prefix failures and bounded geometry probes |
| `profiles/modeled-policies-seed47489.json`, `episode.cpp`, `run_models.py` | Diagnostic native timing-model results and private prototype harness |
| `stage5-video001/sakuya-stage5-lunatic.mp4` | Delivered 88,534,952-byte live Sakuya video |

Prototype probes under `reports/local/` are ignored and are not supported CLI
tools or available in a fresh clone. Promoting a small reproducible harness is
an open task. Raw assets, EXE/DATs, tapes, journals, videos and generated reports
stay private; publish code and carefully scoped summaries only.

## Implementation entry points

| File | Responsibility |
|---|---|
| [scripts/run_wine_shadow.py](../scripts/run_wine_shadow.py) | Owned Wine launch, snapshots, cgroup limits and final receipt |
| [tools/wine-shadow/observer.cpp](../tools/wine-shadow/observer.cpp) | Retail common start, priorities 5/15, input ownership, receipt timestamps and natural terminal |
| [tools/shadow_online.cpp](../tools/shadow_online.cpp) | Actual follower, lead scheduling, catch-up/digest adoption and accounting |
| [include/th06/shadow_bridge.hpp](../include/th06/shadow_bridge.hpp) | Versioned atomic plans, input/lead validation and receipt schema |
| [src/shadow_native.cpp](../src/shadow_native.cpp) | Declared retail projection; positions tolerate 0.025 game units, discrete fields exact |
| [src/shadow_search.cpp](../src/shadow_search.cpp) | Fork group, nominal/pulse/recorded/native portfolio and whole-search deadline |
| [src/shadow_recorded.cpp](../src/shadow_recorded.cpp) | Conditional geometry, bounded index/beam and independent native winner replay |
| [third_party/th06/src/Player.cpp](../third_party/th06/src/Player.cpp) | Minimal read-only headless collision/movement hooks; original source predicates retained |
| [src/planner.cpp](../src/planner.cpp), [src/session.cpp](../src/session.cpp) | Native proposals, complete futures, exact internal digests and lifecycle |
| [scripts/shadow_capture.py](../scripts/shadow_capture.py) | Owned X11 capture, separate audio and timestamp-preserving muxing |
| [tests/shadow_search_tests.cpp](../tests/shadow_search_tests.cpp), [tests/native_geometry_tests.cpp](../tests/native_geometry_tests.cpp) | Native verification, failure/deadline/COW and geometry capacity contracts |

Continue using the local TH08 source/design as reference, with pinned provenance
in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Preserve TH06's own callback
order, collision rules and action-dependent RNG. Cross-runtime float identity is
not a goal; gameplay correctness and exact internal native consistency remain
the acceptance boundaries. Use at most four solver jobs and one Wine instance;
include search, follower, Wine, rendering and recording in resource accounting.
