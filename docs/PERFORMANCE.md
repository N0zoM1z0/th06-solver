# Lunatic hard-stage performance

## Live shadow decision costs

The [Wine shadow MPC](SHADOW_MPC.md) clears two Stage 5 / Reimu B live runs under
the initial 128-tail/16-lead/110-ms profile. A fresh-proposal Stage 6 / Marisa B
run now also clears NMNB: 64 tail, eight lead, 80-ms cap, interval four and a
three-candidate minimum. Its 7,050 searches have median/p99/max
29.07/77.70/82.87 ms including cleanup, 63 timeouts and only four startup fallback
inputs. Search wall time totals 206.55 seconds; controller/child CPU totals
24.31/201.69 seconds over 483.43 seconds of controller wall time. The completed
work counters report 26,255 branches and 1,614,007 updates, explicitly excluding
interrupted work. Faster feedback has a measurable CPU cost; these different-seed
clears do not establish lower total cost or better all-seed success.

Five actual Stage 6 controls fail, including long verification and recorded
geometry. A native timing model of seed 47489 fails under the initial profile,
the fresh profile and a six-candidate profile. Full source rollouts still decide
whether a candidate can supply a queue. Timeout kills and descendant reaping
are charged separately; the previously verified queue keeps retail running.
Default offline policy and cost behavior remain unchanged.

Full retained digests on every failed candidate were an avoidable search cost.
The online planner now hashes the first candidate normally, skips hashes on later
candidates, and independently replays a later winner to build its complete digest
certificate. It charges that verification branch and every replayed update.
Three alternating paired trials at each of four checked Stage 5 seed-51280
origins give these median whole-search times, including winner verification:

| Actual origin, then fixed 16-input lead | Eager hashes | Deferred hashes | Native tail reached |
|---|---:|---:|---:|
| 3,056 | 360.52 ms | 254.47 ms | Complete 128 |
| 8,352 | 466.38 ms | 349.46 ms | Incomplete 63 |
| 13,632 | 392.59 ms | 287.20 ms | Incomplete 104 |
| 17,408 | 106.61 ms | 99.16 ms | Complete 128 |

Literal winner inputs and lookaheads match in every pair. These are diagnostic
windows with the budget disabled to attribute costs, not additional retail
clears or a whole-run speedup claim. Incomplete futures still publish no queue.

The follower also caches its scalar state until an actual receipt changes the
parent world. A synthetic 1,200-receipt stream paced over 20 seconds, with the
same recorded retail inputs and an intentionally unacknowledged proposal,
reduces controller CPU from 0.919 to 0.752 seconds in one pair. It isolates idle
pool rescans rather than establishing live-policy throughput. A truncated final
journal injected into the same fixture is rejected instead of becoming a clear.


This experiment targets standalone stages, not the full matrix or stories. The
previous random campaign stopped in four stage-5 cases; stage 6 consumed the most
search time. The fixed comparison therefore selects stage 5 Reimu B / Marisa B
and stage 6 Marisa B. All starts use Lunatic, power 128 and the source checkpoint.

## Search volume: retain a verified native future

The next source-level waste is overlapping rollout work. Every decision pays
for full native futures but ordinarily keeps only the first eight updates.
With six 120-update candidates, an uncomplicated segment costs roughly 90
speculative updates per accepted update. Retaining 32 already simulated updates
can reduce that ratio toward 22.5 without reducing the horizon or proposals.

`--reuse N` records the selected future's literal inputs and per-update digests.
This includes a short move followed by holding and reactive turns after the
first commit. The parent executes that sequence, checking every digest; repeating
only the first input would be incorrect. Only a full safe horizon or natural
terminal permits extending beyond the original commit. An incomplete future
still supplies at most the legacy eight-update prefix. Bounds are
`commit <= N <= horizon <= 600`; zero remains the default and disables retention.
Both `th06_plan` and the resumable campaign accept the option.

This adapts TH08's complete-certificate/publication-window idea to the offline
TH06 interface. It copies only primitive inputs/digests, never a native world.
All accepted updates still execute normally; gameplay clocks, RNG, physics and
the native kernel are unchanged from `f942133`.

### Development gate and fresh sample

Three known cases each complete NMNB with both 16 and 32 retained updates,
passing the frozen checker's full replay:

| Development case | Speculative updates, reuse 16 | Speculative updates, reuse 32 |
|---|---:|---:|
| Stage 5 Reimu B, seed 7589 | 861,500 | 467,921 |
| Stage 5 Marisa B, seed 5341 | 920,271 | 447,062 |
| Stage 6 Marisa B, seed 7589 | 1,180,865 | 610,957 |

The 32-update variant is frozen before inspecting the baseline outcomes of
eight newly selected seeds: **50102, 1166, 13738, 3693, 10474, 61250, 26526,
22400**. Stage 5 Reimu B / Marisa B use all eight; stage 6 Marisa B uses the
first four. This is another 20 paired cases, separate from the earlier study.
Both versions use horizon 120, commit 8, six proposals, reactive futures,
default candidate ordering and a 60,000-update absolute frame budget.
`--diverse-first` is off in both. The sole experimental policy change is reuse 32.

| Case family | Default initial clears | Reuse-32 initial clears |
|---|---:|---:|
| Stage 5 Reimu B | 7/8 | 7/8 |
| Stage 5 Marisa B | 7/8 | 6/8 |
| Stage 6 Marisa B | 4/4 | 3/4 |
| Total | 18/20 | 16/20 |

The same predeclared recovery budgets apply to both: rewind 768 / horizon 180 /
eight proposals, then rewind 2,048 / horizon 240 / twelve proposals. All six
incomplete attempts clear with the first recovery; the second schedules no work.
Both versions finish **20/20 NMNB**, with every complete route verified in full
by the frozen baseline checker and the final current build.

| All attempts, including failures/retries | Default | Reuse 32 |
|---|---:|---:|
| Attempts | 22 | 24 |
| Search decisions | 57,187 | 14,804 |
| Native branches | 407,147 | 128,083 |
| Speculative native updates | 39,991,255 | 11,297,133 |
| Prefix reconstruction updates | 21,035 | 42,880 |
| Final accepted route updates | 455,861 | 453,098 |
| Planner CPU seconds | 2,659.12 | 935.33 |

Speculative work falls **71.75%**, decisions fall 74.11%, and planner CPU falls
**64.83%** (2.84×). CPU includes all planner attempts, initialization, hashing,
reconstruction and output; checker and diagnostic work are separate. Parent
execution and prefix reconstruction still execute every native update. Final
route lengths differ by less than 1%, so shorter gameplay does not explain
the work reduction. Across all candidate attempts, 337,828 accepted updates
reuse an existing proof beyond the first commit; 10,283 prefixes contain
multiple inputs, confirming that this is not a larger constant-action commit.

The first-pass regression is retained, not hidden by the recoveries. Longer
publication delays the next complete rolling-horizon search and changes later
choices; a safe finite future does not prove long-term escape. Retention remains
opt-in. The results establish a lower-cost bounded offline solve for this sample,
not a universal robustness improvement or an online/Wine controller result.

Eleven focused tests pass. A controlled native callback fails every future at
update 20: the selected safe length is 19 but only eight updates are certified.
Another native fixture checks a mixed-input 32-update prefix and parent/RNG
isolation. Invalid bounds, commit-sized legacy equivalence and campaign option
forwarding are guarded. The whole original 10,567-update default failure tape
also remains byte-identical. Raw evidence is under
`reports/local/lunatic-search-20261025/`.
The solver/probe group peaks at 110,108,672 bytes (105.01 MiB), with zero OOM
events under the same 2-GiB/no-swap cap and at most four solver/probe jobs.

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

## Earlier kernel/diversity comparison

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
  --seed 50102 --power 128 --reactive --horizon 120 --commit 8 --proposals 6 \
  --frames 60000 --reuse 32 --output reports/local/perf-stage5.actions
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

The remaining search bottleneck is the proposal/future family: linear observations
omit TH06's transform and laser state, and short commitments do not certify
long-term escape. Native hazard recording with spatially diverse multi-leg
proposals is a justified next experiment. Persistent checkpoint holders are a
lower priority in the earlier kernel study: prefix reconstruction is roughly
0.1–0.2% of speculative updates. Those counts justify measuring time before adding holder
processes, rather than assuming they are the main search cost.
