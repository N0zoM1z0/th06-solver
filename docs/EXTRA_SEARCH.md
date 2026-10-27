# Extra online search optimization

The 2026-10-27 experiment targets the actual failed Extra Reimu A routes, not a
new offline matrix. Historical seed-0 power-128 checkpoints and normal power-0
Wine starts remain separate. One complete Extra online NMNB clear now passes
for Reimu B / seed 35964 under the combined profile below. Eight earlier full
hits remain counterexamples; all-seed robustness and stable online 60 FPS
remain unestablished. Test counts below identify their milestone builds;
[Validation](VALIDATION.md) owns the current configured test gate.

## Laser guidance and curved proposals

Source inspection and exact failed-input replay found a concrete proposal gap:
`Session::observe` and `proposal_cost` covered bullets/enemies but no lasers.
Both Flandre failures occur at spell 55 / ECL subroutine 37. Immediately before
each hit, nine lasers are present; the principal beam rotates about 0.03927 rad
per update. A distant player cannot simply outrun that sweep with held movement.

Optional `--laser-aware` adds conditional guidance, not a collision replacement:

- Snapshot scalar laser geometry/lifecycle fields. Measure origin/angle changes
  only across continuous same-state timer updates; pooled/restarted lasers and
  phase resets provide no measured motion. Repeated motion is a proposal
  hypothesis and does not predict new spawns or arbitrary future ECL changes.
- Forecast source warning/active/despawn geometry, retaining the original
  thin-length bug and collision on transition updates. Each observation shares
  its forecast across candidate directions and reactive guidance queries.
- Add six native orbital futures around a persistent long beam, including its
  warning/fade interval. Try both directions and bounded inner radii; use measured
  angular rate when available. Each branch periodically reobserves its own laser
  origin and records every actual turn as literal inputs. Full native rollout
  and independent winner replay remain mandatory before publication.

No gameplay manager is copied as another world. Motion history belongs to each
forked native session and is excluded from gameplay digests. The option defaults
off, preserving the previous proposal policy and tape format.

## Controlled native evidence

All raw probes, failed alternatives and binary identities remain under private
`reports/local/extra-search-20261027/`. Guidance-only probes retain failures;
forecasting by itself did not solve the late windows. Curved candidates are a
separate refinement. There are three 44-call fixed-state batches before the
continuous tests; no complete retail clear follows from an individual future.

The continuous model replays the original prefix, starts with 144 captured actual
inputs as its initial queue, preserves a 16-update lead and enforces the same
110-ms search budget. Actual search elapsed time consumes queued actions at a
conservative 60-Hz rate before freshness/lead checks. Subsequent inputs are newly
planned native actions. This is a native latency model, not a Wine run.

| Captured seed / origin | Ordinary model | Laser-aware model |
|---|---|---|
| 29911 / 22,800 | Spell 55 ends at 24,200, no hit | Spell 55 ends at 26,075, no hit |
| 32846 / 22,100 | Hit at 22,426 | Spell 55 ends at 23,471, no hit |

Both guided trajectories have no fallback. All three no-hit model tapes pass the
frozen pre-change checker with exact per-update native digests; the failed control
is rejected. The first row is not a universal policy win: both variants succeed,
and the guided route takes longer. Models retain their changed action-dependent
RNG and do not certify hidden retail state.

The laser-guidance build passed 31 CTest contracts, including actual original
BulletManager collision-call/lifecycle comparisons and literal replay with guidance
history enabled. All four historical Extra tapes still pass complete fresh native
replay. The source and its licenses/provenance are unchanged.

## First Wine validation

```sh
python3 scripts/run_wine_shadow.py \
  --game /path/to/private-game-copy --prefix /path/to/dedicated-win32-prefix \
  --data /path/to/same-six-dat-archives --stage 7 --shot 0 \
  --horizon 128 --lead 16 --budget-ms 110 --interval 4 --proposals 3 --fresh \
  --pulse-on-failure --laser-aware --updates 100000 --seconds 1800 \
  --output reports/local/extra-laser-live
```

`online-laser128-001` receives retail seed 5092 and normal initial power 0. It
passes spell 55 and progresses beyond spell 60, then hits at update 43,521 on spell
62 / ECL subroutine 60. All actual retail projections follow native, with zero
stale/rejected plans. This is a failed complete attempt, not an Extra NMNB clear.
It uses the previous Wine 8 live profile; the separate Wine 11 replay presentation
gate does not establish its FPS.

The controller publishes 10,176 plans, including 334 orbital plans and 84 recovery
plans. There are 266/10,446 search timeouts; p50/p99/max is 44.50/111.41/114.28 ms,
including cleanup beyond the requested cap. Only seven inputs use fallback (four
at startup). Actual receipt p99/max is 29.56/366.48 ms. All processes remain in
the verified 2-GiB/no-swap group, with zero OOM events. Raw inputs, every receipt,
decisions and failure context remain in the private case directory.

At spell 62 the ordinary and recovery searches again time out until the last
certified queue expires. Four fixed-prefix native tests of a fully certified
32-update emergency horizon still hit. Separate 64/240/320-horizon controls also
hit under the existing budget; the longer horizon produces more timeouts. These
negative experiments are retained and are not shipped as a proven rescue.

Six general center-orbit recovery candidates also fail from both captured
43,000/43,300 origins. Prioritizing distinct input directions instead of repeated
tails does not solve either 42,600/43,000 comparison. From 42,600, the ordinary
control reaches the 1,000-search limit at 46,621 without a hit; it has not ended
the spell. A fresh extended retry hits at 44,180. Both use measured search wall
time, so scheduling and resulting action-dependent worlds can differ between
runs. The no-hit prefix passes the frozen checker; the failed retry is rejected.
All retries remain in the ledger. Neither is a complete spell/Extra clear.

## Remaining search work

The next target is spell 62's approach to the right wall, before repeated failed
searches exhaust the queue. A smaller timeout count alone is not acceptance:
candidate ordering must also leave a viable continuation. Fixed-horizon native
verification and actual lead/deadline checks remain mandatory.

Optional `--anytime` now preserves the first full native future while further
ranking continues. Its immutable snapshot contains the fixed lead, literal
inputs and every native digest. A deferred-hash candidate receives independent
literal replay before notification. The shared publication word is released only
after the entire snapshot and completion timestamp; no subsequent write touches
it. On timeout the parent first kills/reaps the whole search group, then reads
the snapshot only if it completed within the requested budget. Normal completed
searches still return their final winner. Retail lead/freshness rejection remains
unchanged; a deadline checkpoint is not automatically a published plan.

The timeout remains counted and measured, including cleanup. Interrupted work after the
checkpoint is excluded from completed branch/update counters. The option defaults off.
The anytime build passed 34 CTest contracts, including a full first candidate followed
by a deliberately blocked nested rollout, default-off rejection,
failed/unfinished-future rejection and independent literal digest replay after all
descendants are reaped.

The later private `extra-online-20261027` probes retain further failures:
conditional geometry recovery and two bounded native feedback-tree variants do
not solve late spell-62 windows. From the earlier 42,600 origin, ordinary and
conditional-geometry latency models both end spell 62 at 47,416 without hits;
both tapes pass the frozen pre-change checker. That success is timing-dependent,
not a paired robustness claim or a retail clear. Generic continuation-aware
ranking before the edge trap remains open.

## Long-uptime live limiter

Three attempted validations produce zero gameplay receipts: two hit the existing
30-second startup timeout; the third is stopped after read-only diagnosis. The
original limiter is stuck subtracting 16.6667 from 538,207,616 in 24-bit x87
precision, which rounds back to the same value. Its absolute timestamp is already
536,870,912 ms. This is a startup failure, not an MPC route failure; all three
attempts and the exact FPU/memory evidence remain private.

The live launcher now accepts `--precise-limiter` and `--sleep-limiter`, reusing
the replay-tested arithmetic scope. Installation occurs on the first limiter
call, before the first menu callback, so it also handles startup catch-up. Only
the pinned limiter call widens to 53 bits. BeginScene/Present restore the exact
prior control word; menu, init, sampling and observation callbacks reject a
leaked scope. The optional sleep retains the original final checks and clock
values. Lightweight forwarding wrappers do not collect render timings. The
startup wait remains 30 seconds and the MPC budget is unchanged. The actual
stuck subtraction is covered by the asset-free x87 regression.

## Cooperative ranking and native power pickups

With `--anytime`, ranking can now yield after 75% of the requested whole-search
budget, but only once a full native future exists. The remaining time is for
literal winner verification and cleanup; it is a reserve, not a separate budget
or a guarantee that one native branch finishes in time. The hard group deadline
and first-certificate fallback still apply. `--proposals` becomes a best-effort
candidate target under this option. Returned ranking yields have their own
counter. An expired soft deadline cannot publish a failed/unfinished future.

Optional `--power-aware` adds pickup attraction and a native power-deficit cost
until power reaches 128. The nearest actual small/large/full-power item guides
horizontal proposals; bomb/life/point items and the source's failed-spawn dummy
are excluded. Item motion, collision and rewards still run in original
ItemManager. No remaining item means no attraction penalty, including just after
collecting the last one. Full power restores the original ranking/scoring policy.
The power-guidance build passed 36 CTest contracts, including original large-item pickup (64 to at
least 72), independent input/digest replay and identical full-power routes.

Two captured-seed opening latency controls start at update 256 with the same
144 queued actual inputs. At about 10,250 updates, the original policy has power
64/73 for seeds 13662/5092; the promoted pickup implementation has 128/128, with
zero hits/fallback. Both promoted tapes pass the frozen pre-change checker.
These are native prefixes, not normal-start Wine clears. The first pickup
prototype's mixed 53/81 result remains in the private ledger.

For the spell-58 failure, a six-candidate soft-deadline prototype ends the spell
from both 35,800/36,300 origins; its hard-ranking controls hit at 36,064 and end
at 37,453 respectively. Both soft tapes pass frozen fresh replay. A separate
320-tail/anytime model from the old spell-62 origin 43,000 ends the spell at
47,416 with 25 deadline checkpoints and no fallback; its control hits at 43,880.
The earlier 42,600-origin 320-tail controls still fail. Field-orbit, four-edge
penalty and native feedback-tree alternatives show no reliable advantage and
remain unshipped. All failures/retries are retained.

The complete `online-anytime128-004` Wine attempt (seed 13662, normal power 0)
passes spell 55, then hits at 36,587 on spell 58 / ECL subroutine 48. All retail
projections and independent native digests agree, including the hit. It publishes
8,707 plans from 8,888 searches, with 179 timeouts, one recovered deadline
certificate and five fallback inputs (four startup). Search p50/p99/max is
41.21/111.43/117.28 ms including cleanup. All 36,706 limiter scopes restore;
296,895 coarse sleeps occur. Peak is 475,500,544 bytes (453.47 MiB), OOM 0 under
the verified 2-GiB/no-swap cap. This is another failed full attempt, not a clear.

## Survival-phase lookahead

Optional `--survival-horizon 256` uses a longer certified future when an active
spell's present bosses are all non-damageable, non-collidable and
non-interactable. This is a native state cue, not a hardcoded stage/spell/seed
check. Otherwise the base horizon applies. The hard search budget, immutable
lead and verification rules do not change. Bounds require base <= survival
horizon and survival horizon + lead <= 600; recorded geometry also retains its
240-update limit. Every decision logs its actual horizon, and the controller
counts extended searches/publications. Zero disables the option.

From just before that cue in two actual failed spell-62 prefixes, the ordinary
128-tail controls hit at 40,110/43,500 for seeds 59127/5092. The 256-tail variant
ends the latest seed's spell at 43,999 without hit/fallback, but the older seed
still hits at 43,539. Both 320-tail variants hit. The promoted 256 implementation
repeats the latest phase completion with 19 deadline checkpoints and no fallback;
its entire 43,999-update native prefix passes the frozen checker. Thirty-six
CTest contracts still pass. This is a targeted native improvement, not a robust
all-seed solution or a full retail clear.

The preceding complete `online-power6-005` Wine attempt (seed 59127, normal
power 0) reaches full power by update 5,536, passes spell 58, then hits at 40,028
on spell 62 / ECL subroutine 60. All actual digests independently replay. It
publishes 8,020 plans from 8,242 searches, with 319 timeouts, 100 published
deadline checkpoints, 224 ranking yields and ten fallback inputs (four startup).
Search p50/p99/max is 69.18/111.41/114.10 ms. All 40,147 limiter scopes restore;
313,616 coarse sleeps occur. Peak is 373,649,408 bytes (356.34 MiB), OOM 0 under
the verified 2-GiB/no-swap cap. These are combined-profile, different-seed results;
they do not establish universal performance improvement or stable online FPS.

A normal-start validation profile is:

```sh
python3 scripts/run_wine_shadow.py \
  --game /path/to/private-game-copy --prefix /path/to/dedicated-win32-prefix \
  --data /path/to/same-six-dat-archives --stage 7 --shot 0 \
  --horizon 128 --survival-horizon 256 --lead 16 --budget-ms 110 \
  --interval 4 --proposals 6 --fresh --pulse-on-failure --laser-aware \
  --anytime --power-aware --sleep-limiter --updates 120000 --seconds 2400 \
  --output reports/local/extra-survival-live
```

The subsequent `online-survival256-006` Wine validation captures seed 24237.
It hits at 28,121 on spell 58 / ECL subroutine 48, boss HP 386; the survival cue
has not activated, so extended searches/publications are both zero. All retail
projections and independent actual native digests agree including the hit.
There are 6,398 searches, 132 timeouts, 6,289 publications, 26 published deadline
checkpoints, 47 ranking yields and five fallback inputs (four startup).
Search p50/p99/max is 55.39/111.34/113.32 ms including cleanup; receipt gap
p99/max is 25.99/577.41 ms. All 28,240 limiter scopes restore. Group peak is
366,788,608 bytes (349.80 MiB), OOM 0 under verified 2-GiB/no-swap limits.
This is the seventh completed full Extra hit, not a survival-horizon success.

## Actual boss damage as a proposal goal

The latest spell-58 capture has dense nearly radial spokes about (192, 224),
not a persistent beam with directly measured angular velocity. Instantaneous
bullet velocity does not reliably supply an orbital direction. General field
orbits, outer-radius alternatives and edge penalties remain private experiments:
they add timeouts and show no reliable advantage across the captured windows.

Optional `--damage-aware` instead scores actual native boss HP relative to the
initial active damageable spell, and favors horizontal target alignment in
cheap proposal ordering. Native `EnemyManager::OnUpdate` gates shot damage on
interaction, visibility, prior in-bounds state and damageability; body collision
is a separate condition. The observation follows those source gates. Completing
the initial phase latches its goal to zero even if the next phase resets HP or
reuses the spell identifier. Invulnerable survival phases supply no HP goal.
Only proposal ranking changes: collisions, death, drops, phase transitions and
full-horizon verification still execute in the original native runtime.

A private prototype ends spell 58 in three captured 128-tail latency windows;
all three full-prefix tapes pass the frozen checker. Repeating with the promoted
implementation retains an important failure. Each query still has a 110-ms hard
cap, 16-update lead, six-candidate target, fresh/anytime/power-aware policy and
144 captured initial queued inputs:

| Captured seed / origin | Promoted 128-tail damage goal | Promoted global 256-tail damage goal |
|---|---|---|
| 24237 / 27,300 | Spell ends at 28,514, no hit/fallback | Hit at 27,480; 24/27 timeouts, 36 fallback |
| 24237 / 27,900 | Hit at 28,463; 49/94 timeouts, four fallback | Hit at 28,247; 47/51 timeouts, 33 fallback |
| 13662 / 36,300 | Spell ends at 37,454, no hit/fallback | Hit at 36,674; 51/55 timeouts, 11 fallback |

Both successful promoted prefixes pass independent frozen replay with exact
per-update native digests. A separate 256-tail opening model reaches update
10,256 with power 128 and no hit/fallback, also frozen-replayed; that opening
does not excuse the three dense-window failures. Global 256 is therefore not
adopted. Timing-dependent model retries are retained, not pooled as independent
retail wins. The damage flag remains optional and experimental.

The damage-guidance build passed 38 local CTest contracts, including
phase-reset/identifier-reuse goals, invulnerable phases and the actual native
damage-context gates. Existing source, physics, input restrictions and replay format are
unchanged. Current raw model, failed-control and actual-retail evidence is under private
`reports/local/extra-online-20261027/`; the next target remains viable earlier route
choices before the edge traps, within the actual decision deadline.

The `online-damage128-007` actual Wine attempt captures Reimu A seed 5594,
normal power 0. It passes spell 55, then hits at update 22,062 on spell 57 /
ECL subroutine 44, boss HP 946. Every actual native digest and retail projection
independently matches including the hit. It publishes 4,854 plans from 4,921
searches, with 81 timeouts, 18 published deadline certificates, 47 ranking yields
and eight fallback inputs (four startup). Search p50/p99/max is
59.45/111.08/112.51 ms including cleanup; receipt p99/max is 27.53/381.51 ms.
Group peak is 398.15 MiB, OOM 0 under verified 2-GiB/no-swap limits. All 22,181
limiter scopes restore. This eighth full hit does not establish damage-guidance
robustness or a complete Extra online clear.

## Finite native bullet acceleration

That spell-57 capture contains 38–132 bullets with `EX_ACCELERATION` at selected
samples. Constant-velocity ranking omits their remaining acceleration. New
optional `--acceleration-aware` snapshots acceleration only for fired bullets,
with the source's finite `exDuration` and current timer. Speedup, angular-turn,
velocity-change and bounce combinations are excluded; spawn animation remains
outside this predictor. Position forecasting applies acceleration before motion,
then retains the accumulated velocity after expiry. Forecasts are proposals,
not collision certificates, and do not predict new bullet spawns or ECL changes.
All native state, callbacks and RNG remain unchanged.

The acceleration-guidance build passed 40 local CTest contracts. An actual BulletManager
fixture initializes original bullets and checks twelve callback updates across three
acceleration lifetimes, expiry and source speedup precedence. Guidance-disabled
observations retain the original linear forecast; snapshot reads preserve gameplay
digests.

Two 128-tail/16-lead/110-ms captured spell-57 latency comparisons retain all
outcomes, with six-candidate fresh/anytime/power/damage policy and the same
initial 144 queued actual inputs:

| Seed 5594 origin | Acceleration off | Acceleration on |
|---|---|---|
| 21,200 | Spell ends 22,129; 6/182 timeouts | Spell ends 22,839; 10/323 timeouts |
| 21,700 | Spell ends 24,048; 27/428 timeouts | Spell ends 22,890; 33/219 timeouts |

All four have no hit/fallback and pass full-prefix frozen replay. The controls
also succeed, and the earlier guided model is slower; this establishes tested
motion semantics and mixed targeted behavior, not a general policy win or retail
clear. Raw attempts and checks remain under private
`reports/local/extra-online-20261027/acceleration-model001/`.

## First complete Extra online NMNB clear

`online-acceleration-reimuB-008` captures retail seed **35964**, original Extra
power 0, Reimu B. Actual Wine reports one natural clear at **43,126 updates**,
zero misses and zero bombs. A fresh independent process using the frozen
pre-change checker with `--require-complete` replays the entire actual tape,
matches every per-frame native digest and confirms complete NMNB. Independent
source-context replay matches every actual retail projection, ending at spell
63 / ECL subroutine 69, boss HP 0. No hidden-state import or offline replacement
route is used.

The combined profile is implemented at
[`4750259`](https://github.com/N0zoM1z0/th06-solver/commit/475025965d12a6ccb4123c762251ed61340d31b8),
with [green implementation CI](https://github.com/N0zoM1z0/th06-solver/actions/runs/40494057560)
and all 40 local CTest contracts passing:

```sh
python3 scripts/run_wine_shadow.py \
  --game /path/to/private-game-copy --prefix /path/to/dedicated-win32-prefix \
  --data /path/to/same-six-dat-archives --stage 7 --shot 1 \
  --horizon 128 --survival-horizon 256 --lead 16 --budget-ms 110 \
  --interval 4 --proposals 6 --fresh --pulse-on-failure --laser-aware \
  --anytime --power-aware --damage-aware --acceleration-aware --sleep-limiter \
  --updates 120000 --seconds 2400 --output reports/local/extra-combined-live
```

This run uses the existing Wine 8 live profile, no recording, and scoped limiter
precision/coarse sleeps. Search and source receipts retain their actual timings:

| Measure | Actual result |
|---|---:|
| Searches / timeouts / publications | 9,590 / 360 / 9,307 |
| Published deadline certificates / ranking yields | 81 / 207 |
| Survival-horizon searches / publications | 1,063 / 966 |
| Orbital / conditional recovery publications | 265 / 67 |
| Planned / fallback inputs | 43,122 / 4, all fallback at startup |
| Stale / rejected plans / follower errors | 0 / 0 / 0 |
| Search p50 / p99 / max, including cleanup | 59.54 / 111.27 / 119.00 ms |
| Actual receipt mean / p99 gap / max gap | 57.08 Hz / 27.49 ms / 628.02 ms |
| Controller wall / whole service wall | 755.46 / 765.09 s |
| Whole Wine-tree peak memory | 359,501,824 bytes, 342.85 MiB |
| Verified memory / swap / OOM / OOM-kill | 2 GiB / 0 / 0 / 0 |
| Limiter entries / restores / coarse sleeps | 43,245 / 43,245 / 287,506 |

All frozen binary hashes match the resource receipt. The actual-input tape's
SHA-256 is `98025c911e03c56225a83819ac3461eeb76d4b39fbeff3b77bab1f8acd9aee61`.
The case's `proof.json`, full actual inputs, every retail receipt, all decisions,
resource/binary identities and independent checker outputs remain private under
`reports/local/extra-online-20261027/`. Generated Extra tapes remain ignored;
the tracked Lunatic replay is unchanged.

This is **one complete combined-profile/shot/seed clear**. The ledger still
includes eight failed full attempts, one incomplete prefix and three zero-frame
startup attempts. Different shots/seeds and timing-dependent model outcomes do
not establish a single-change speedup or general robustness. Stable online
60-FPS presentation also remains unestablished. Next work should test selected
hard seeds and earlier route choices, while measuring online pacing separately;
do not rerun the full matrix or discard the retained counterexamples.
