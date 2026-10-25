# Lunatic hard-stage performance

This experiment targets standalone stages, not the full matrix or stories. The
previous random campaign stopped in four stage-5 cases; stage 6 consumed the most
search time. The fixed comparison therefore selects stage 5 Reimu B / Marisa B
and stage 6 Marisa B. All starts use Lunatic, power 128 and the source checkpoint.

## Why a safe search can stop

`src/planner.cpp` ranks linear forecasts of current bullets and enemy bodies.
That cheap model omits future spawns, nonlinear bullet transforms and laser
geometry. TH06's ECL and bullet callbacks implement those behaviors, and each
forked native future executes them with its own action-dependent RNG. Native
verification catches unsafe predictions; the ranking is not a safety oracle.

The candidate family is also limited: a short move then hold, a continued move,
or a reactive continuation that replans every 16 updates with a short forecast.
Different tails often certify exactly the same initial eight inputs. With six
rollouts per decision, the old ordering can cover only a few initial actions.
In fixed 80-update probes, 60 rollouts cover 40 distinct initial inputs in stage 5
and 30 in stage 6. `--diverse-first` covers 60 in both, without raising the budget.
It retains the best-ranked tail per input before evaluating duplicate tails.

When all evaluated futures hit within the horizon, the planner can still commit
their longest consecutive safe prefix. This certifies those updates, not escape
from a later trap. Eventually every tested first action can fail immediately.
The earlier stage-5 seed-5341 counterexample reproduces precisely that behavior;
replaying and rewinding an earlier accepted state finds another route. A search
failure therefore does not mean the native level is broken or globally unsolvable.

The diversity idea follows `th08-solver`'s proposal-coverage work, adapted to the
TH06 prefix interface. It is optional because it trades future variety for initial
action variety. On the development case stage 5 Reimu B, seed 7589, it clears at
22,137 updates with horizon 120 / six proposals, without rewinding. The earlier
policy needed two recoveries and 4,328,785 speculative updates; this route needs
1,913,414. Stage 5 Marisa B, seed 5341 still stops at 11,150, so this is not a
general solution. Search expansion, incomplete prefixes and all retries stay counted.

## Source hotspots and retained changes

Eight fresh-process `-pg` replays of stage 6 Marisa B, seed 7589 pass terminal
digest checks. Diagnostic self samples place ANM execution at 51.41%, graze at
13.23% and bullet update at 8.28%. The samples identify code to inspect; their
instrumented times are not production benchmark results.

Two small source-preserving optimizations remain enabled:

- ANM alpha interpolation normally retains RGB. Equal byte endpoints are exact
  identities for the finite timer fraction, so their redundant x87 conversions
  can be skipped. Changed components, clocks, dispatch and RNG still execute.
- Player update writes Bomb regions at callback priority 7. The bullet callback
  at priority 11 reads them after item update. A stack-scoped, owner-bound empty
  result skips the repeated sixteen-region scan in graze/collision queries.
  Active regions and calls outside that scope retain the source loop. Player
  writes invalidate matching scopes, including nested scopes. No cached world,
  actor pointer graph or RNG state is shared between native branches.

The scoped cache adapts the same lifetime discipline as TH08's cancel-region
cache, using TH06's actual `size.x == 0` predicate and callback boundaries.
`TH06_FAST_ANM_COLORS` and `TH06_FAST_BOMB_REGIONS` can be independently disabled
in CMake. An idle-ANM interpreter shortcut measured only 0–1% and was removed.

Three alternating fresh-process samples per variant use complete, unchanged
accepted tapes. Native time excludes resource initialization, digesting and I/O:

| Workload | Original median (s) | Bomb cache only (s) | Both (s) | Original / both |
|---|---:|---:|---:|---:|
| Stage 5 Reimu B, seed 7589 | 0.526446 | 0.429047 | 0.334115 | 1.58× |
| Stage 6 Marisa B, seed 7589 | 1.574781 | 1.432371 | 0.825954 | 1.91× |

An independent earlier paired batch measures 1.55× / 1.98×. All samples and
terminal checks are retained. The second eight-replay diagnostic keeps identical
native call counts; ANM/graze self shares fall to 26.20% / 7.07%. This confirms
which work was removed, without treating profiler overhead as application speed.
The optimized default search also reproduces the entire original 10,567-update
Reimu B failure tape byte-for-byte, preserving the baseline policy and replay.

## Fixed random-seed comparison

Eight new seeds are chosen before gameplay trials:
**54269, 14916, 31166, 20637, 33164, 54773, 14984, 1145**. Both stage-5 shots use
all eight; stage 6 uses the first four. These are 20 paired cases, not 96 cases.
Known earlier failures are development cases and are excluded from this sample.
The candidate implementation is frozen before inspecting baseline outcomes.

Both versions use reactive futures, horizon 120, commit 8, six proposals and a
60,000-update absolute frame budget. The candidate combines the two native fast
paths with opt-in diverse-first ordering. Both receive the same predeclared
recoveries: rewind 768 / horizon 180 / eight proposals, then rewind 2,048 /
horizon 240 / twelve proposals. Failed attempts remain archived. The original
frozen checker verifies every accepted route and incomplete prefix in a new
process, checking every digest and zero misses/bombs.

| Case family | Original initial clears | Diverse-first initial clears |
|---|---:|---:|
| Stage 5 Reimu B | 3/8 | 5/8 |
| Stage 5 Marisa B | 7/8 | 8/8 |
| Stage 6 Marisa B | 4/4 | 3/4 |
| Total | 14/20 | 16/20 |

Diverse-first helps this stage-5 sample and regresses one stage-6 case. It remains
opt-in; the native fast paths do not change the default candidate policy. Every
incomplete sample clears with the first recovery, so both versions finish
**20/20 NMNB**. The second recovery schedules no work. All 40 complete tapes also
pass full fresh replay under the final current build, in addition to the frozen
original checker used for each attempt.

| All planner attempts | Original | Fast kernels + diverse-first |
|---|---:|---:|
| Attempts, including failures | 26 | 24 |
| Planner CPU seconds | 3,622.66 | 2,744.31 |
| Native branches | 418,276 | 438,985 |
| Speculative updates | 42,326,985 | 42,232,543 |
| Prefix replay updates | 71,443 | 46,379 |
| Final accepted route updates | 447,681 | 455,355 |

This fixed complete-outcome comparison uses **24.25% less planner CPU** (1.32×).
CPU includes initialization, forks, reconstruction and tape output for every
planner attempt; checker and diagnostic workloads are separate. Speculative work
falls only 0.22%, and branch count rises. The end-to-end gain is mainly a faster
native kernel, not a demonstrated general reduction in search complexity.

| Case family | Original planner CPU (s) | Candidate planner CPU (s) |
|---|---:|---:|
| Stage 5 Reimu B | 1,466.94 | 1,164.05 |
| Stage 5 Marisa B | 1,039.36 | 827.19 |
| Stage 6 Marisa B | 1,116.36 | 753.07 |

The scoped process group peaks at 190,259,200 bytes (181.45 MiB), including
bounded diagnostic/build work, with zero OOM events under its 2-GiB/no-swap limit.
Nine focused tests pass: the eight asset-free contracts plus a real-DAT stage-5
profiling/replay guard. Tests cover nested cache ownership/invalidation, bomb
collision priority, independent alpha/RGB expected values and profiling that
retains the native profile identifier and exact input/digest tape.

## Reproduction and limits

```sh
build/th06_plan --data /path/to/th06-data --stage 5 --difficulty 3 --shot 1 \
  --seed 54269 --power 128 --reactive --horizon 120 --commit 8 --proposals 6 \
  --frames 60000 --diverse-first --output reports/local/perf-stage5.actions
build/th06_replay_check --data /path/to/th06-data \
  --tape reports/local/perf-stage5.actions --require-complete
build/th06_runtime_bench /path/to/th06-data reports/local/perf-stage5.actions
```

`--profile` adds parent/child CPU and phase times under `timings`. Instrumentation
is optional and separately checked to preserve the input/digest tape and native
profile identifier. Phase times explain a fixed short probe; they are not added
to whole-search elapsed time. The runtime benchmark checks its terminal digest;
it does not replace full fresh replay. Each benchmark repetition uses a new
process, respecting the native runtime's one-game-per-process contract.

Measurements use Clang 14, Linux i386, RelWithDebInfo (`-O2 -g -DNDEBUG`), no
fast-math, and three concurrent solver jobs on a shared host. All experiment
descendants share a 2-GiB/no-swap slice; a fourth slot is reserved for sequential
diagnostic probes. CPU, wall, speculative and prefix-replay costs remain distinct.
Callgrind's x87 emulation diverged from a reference digest and was excluded;
native execution provides the timing and correctness evidence instead.

Raw protocols, frozen binaries, attempts, tapes and receipts remain ignored under
`reports/local/lunatic-perf-20261025/`. This fixed sample is native evidence, not
a proof for every seed or new Wine playback evidence. Cross-runtime comparison
accepts harmless floating-point drift while requiring correct gameplay progress.

The next search bottleneck is the proposal/future family: linear observations
omit TH06's transform and laser state, and short commitments do not certify
long-term escape. Native hazard recording with spatially diverse multi-leg
proposals is a justified next experiment. Persistent checkpoint holders are a
lower priority here: prefix reconstruction is roughly 0.1–0.2% of speculative
updates. Those counts justify measuring reconstruction time before adding holder
processes, rather than assuming they are the main search cost.
