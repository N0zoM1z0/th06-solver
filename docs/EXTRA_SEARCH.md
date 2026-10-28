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

## 2026-10-28 stability and deadline study

Private evidence is under `reports/local/extra-stability-20261028/`. This study
keeps fixed-prefix native probes, timing-dependent native policy models and
new actual Wine trials separate. All shots in the new Wine trials are Reimu B
with natural power-0 Extra entry. No historical matrix/story campaign was rerun.

The maintained `th06_shadow_probe` checks the DAT identity, every prefix/lead
digest and independent literal execution of each returned certificate. Six
captured seeds (24443, 5594, 24237, 13662, 59127, 35964), three alternating
before/after pairs each, produce identical prefix-plus-certificate tapes after removing
unused scores from literal winner verification. All 36 tapes pass a frozen
fresh checker. These 2,000-ms probes isolate route equivalence from the 110-ms
online deadline; their noisy wall times do not establish an online speedup.

A separate repeated-origin benchmark on seed 5594 / origin 21,700 uses 30
`choose` calls per run, ordered before/after/after/before. Child CPU averages
2.191665 s before and 1.797685 s after (18.0% lower); all four runs have identical
input/digest checksums, 210 branches and 19,110 native updates. This measures
one native workload under shared-host load, not complete-game or online FPS.

Periodic nominal extension (`--refresh-updates 32`) is retained as an opt-in
experiment, following th08. Its three captured-window latency controls use the
same initial 144 actual queued inputs, horizon 128 (256 for spell 62), lead 16,
cap 110 ms and six-candidate target:

| Captured seed / origin | Fresh control | Refresh every 32 updates |
| --- | --- | --- |
| 5594 / 21,700 | Spell 57 ends at 22,595, no hit/fallback | Hit 22,082, one fallback |
| 24237 / 27,900 | Spell 58 ends at 28,431, no hit/fallback | Hit 28,129, one fallback |
| 59127 / 39,700 | Hit 39,984, 12 fallback | Hit 39,984, two fallback |

Nominal extension reduces repeated search work but fails the first two controls;
it is not a recommended replacement for the successful fresh profile. A separate
six-run native latency batch exposes the existing planner's `--diverse-first`
ordering. All six runs hit: seed 5594 ends at 21,990/21,982, seed 24237 at
28,078/28,121 and seed 13662 at 36,690/36,803 (ordinary/diverse). The last diverse
case reduces fallback from 38 to two, but still fails NMNB. These timing-dependent
models use `ceil(search_ms * .06)` consumed updates; host load changes their
trajectories. They are counterexamples, not paired retail wins.

The first actual new-seed baseline retains the prior combined profile and
captures seed **24443**. It hits during the midboss section at **10,575**:
2,391 searches, 606 timeouts, 2,170 publications, 11 fallback and no stale or
rejected plan. The last publication originates at 10,424 and ends at 10,568.
Independent source-context replay matches all retail projections; frozen
diagnostic replay matches every digest including the hit. Build/tests overlapped
this baseline. The interactive launcher was interrupted before its final resource
receipt: systemd retains configured 2-GiB/1.5-GiB/no-swap limits, but measured peak
and OOM counters are unavailable. This failed route has incomplete resource proof
and is not used as a clean pacing comparison. The runner now atomically snapshots
measured resources every second and explicitly labels unfinished receipts.

The second actual trial (`fresh3-002`, seed **4902**) keeps the combined profile
with three candidates instead of six. It hits at **38,787**, spell 62 / subroutine
60. All retail projections and native digests match frozen independent replay.
There are 9,196 searches, 2,443 timeouts, 7,670 publications and nine fallback
inputs (four startup). Search p50/p99/max is 89.74/147.14/267.84 ms including
cleanup; actual receipt mean is 31.94 Hz, p99/max gap 78.18/635.97 ms. The
verified 2-GiB/1.5-GiB/no-swap tree peaks at 393.63 MiB with OOM 0.

Its first survival query is at 37,689. The last publication, a recovery from
38,510, certifies through 38,782. The following **61** replacement searches fail
(57 timeouts); five exhausted-queue fallback inputs then end in the hit. No plan
is stale/rejected and the native follower reports no error. This is an earlier
route/continuation failure, not a collision-model disagreement or certified rescue.

Nine maintained fixed-prefix probes use origins 37,700/38,100/38,500 and
256-tail/110-ms, 256-tail/200-ms or 320-tail/200-ms profiles, each with lead 16
and a three-candidate target. Every probe returns a complete certificate; all
nine prefix-plus-future tapes pass frozen fresh replay. Three subsequent native
latency episodes from 38,100 retain 144 initial actual queued inputs:

| Tail / cap | Outcome | Timeouts / searches | Fallback |
| --- | --- | --- | --- |
| 256 / 110 ms | Spell ends 42,728, no hit | 210 / 809 | 0 |
| 256 / 200 ms | Hit 38,894 | 21 / 92 | 4 |
| 320 / 200 ms | Spell ends 42,728, no hit | 72 / 602 | 77 |

All three complete generated traces independently replay, including the failed
trace's hit. These are phase endpoints, not game clears. The larger-cap failure
and the longer-horizon fallback rule out claiming a general recovery. The last
model overlaps the functional rendering smoke below; all latency models remain
host-timing dependent. Fixed-prefix probe start hashes are recorded, but the
initial paired study used a mutable build path for its after binary; the frozen
output tapes, independent checker and separately copied benchmark binaries remain.

The deadline regression then proves a separate protocol bug: if the parent is
stopped while search finishes after its cap, the old code accepts a readable
40-update certificate with `timeout=false`. The repaired search checks the
child's final completion timestamp, rejects the late result, kills/reaps the
group and admits only an independently complete in-budget anytime checkpoint.
Both anytime settings pass the adversarial test; all **43** local CTest contracts
pass. The two full new Wine failures above predate this guard and retain their
original timing evidence.

The new optional live `--render-timing` uses the existing mapped replay journals
to measure actual update/draw/Present API clocks. `render-smoke-003` captures
seed **61531** and ends at its 1,200-update budget with no miss/bomb. Frozen
native replay and retail projection checks pass. There are 1,199 Present records,
no skipped sequences or draw/error/repeat flags, and 1,319 limiter entries and
restores. Resource limits are verified at 2 GiB / 1.5 GiB / no swap, peak
335.54 MiB, OOM 0. This is an incomplete functional prefix under shared CPU load.

Full Present cadence is 40.64 Hz / slowest five seconds 31.4 Hz / p99 58.92 ms;
the unchanged pacing/acceptance gates fail. After the explicitly reported
120-update warmup, draw mean/p99 is 0.76/1.32 ms and Present-call mean/p99 is
19.18/51.65 ms. Most measured wall time is inside Present in this opening; that
does not distinguish driver waits from host scheduling or certify window FPS.
The next pacing investigation should inspect that boundary alongside online
search, retaining full clocks and an independent window observer.

This follow-up adds **two failed full retail attempts, one incomplete retail
prefix, 45 fixed-prefix probe tapes and 15 closed native models**. Every failed
retry remains in the private ledger. The original verified clear remains the
only full Extra online clear; random-seed robustness and stable online 60 FPS
are still open.

## 2026-10-28 verified backup and shorter futures

Private evidence: `reports/local/extra-control-20261028/`. Sharing scalar bullet
forecasts across root/reactive proposal inputs preserves six captured input/digest
tapes exactly. A before/cache/cache/before 30-query workload has identical branch,
update and checksum totals and 4.0% lower mean child CPU. This is native workload
evidence, not online FPS. O3 also passes the complete old clear replay but is
slower in that microbenchmark; deployment retains O2.

Optional `--warm-start` verifies an extended nominal backup before fresh search.
Fresh certificates take priority. A timeout can recover only a complete,
immutable, in-budget checkpoint after whole-group cleanup. A selected warm backup
still arms the next one-shot motion recovery. `--rescue-horizon 64` tries a full
shorter native future after an unsuccessful replacement when the queue reaches
lead + half the normal horizon; successful publication retries the normal horizon.
It never publishes a partial failed rollout. All **47** configured CTest contracts
pass, including deadline, warm-backup and acknowledged-queue corruption checks.

Four private reactive-cadence models and four earlier warm-backup prototype
models retain mixed outcomes and no demonstrated general policy win. Six further
110-ms latency models compare rescue off/64 with lead 16 and six proposals:

| Seed / origin / initial queue | Rescue off | Rescue 64 |
| --- | --- | --- |
| 27718 / 23,190 / exact captured queue | Phase 58 ends 25,024; no hit/fallback | Ends 25,021; no hit/fallback; 7 short publications |
| 27718 / 23,904 / exact captured queue | Hit 24,189; 4 fallback | Phase ends 24,979; no hit/fallback; 4 short publications |
| 59127 / 39,700 / synthetic 144-input queue | Hit 39,984; 6 fallback | Phase 62 ends 43,999; no hit/fallback; 6 short publications |

Every rescue-model trace independently matches all native digests, including hits;
all four surviving prefixes also pass strict fresh replay. These are phase
endpoints, not full clears. All six start the proposal cursor at zero and motion
recovery unarmed rather than claiming the original planner state; game RNG follows
the exact actual prefix. Latency consumption is `ceil(search_ms * 0.06)`, so these
models remain host-timing dependent. The earlier eight prototype episodes also
independently replay with matching digests, including their two hits; their
prototype behavior remains separate. Every retry remains in the ledger.

Three actual Wine trials then use Reimu B, natural power-0 Extra, the 128/256-tail,
16-lead, 110-ms, interval-4, six-proposal combined profile, with warm backups,
render clocks and full certificate capture. No build/native experiments run
alongside these measurements:

| Trial / seed | Actual outcome | Searches / timeouts / publications | Fallback | Peak / OOM |
| --- | --- | --- | --- | --- |
| `online-warm-001` / 27718, rescue disabled | Hit 24,033, spell 58 / sub 48 | 4,884 / 732 / 4,742 | 4 startup + 1 exhausted | 497.00 MiB / 0 |
| `online-rescue-002` / 61515, rescue 64 | **NMNB clear 44,959** | 9,045 / 1,721 / 8,719 | 4 startup only | 447.27 MiB / 0 |
| `online-rescue-003` / 57265, same profile | **NMNB clear 47,986** | 10,066 / 2,818 / 9,432 | 4 startup only | 388.54 MiB / 0 |

All three have verified 2-GiB hard / 1.5-GiB high / no-swap limits. The first hit
matches every independent native digest and retail projection. Its final
certificate ends at 24,032. The second has a complete frozen `--require-complete`
replay with every digest matching, original terminal / spell 63 / sub 69 / HP 0,
zero misses/bombs and matching retail projections throughout. Its tape SHA-256 is
`e8ceb854fb043ec84997e59db25b45db69068e0962633b461d133d78cb2f6d6a`.
Every acknowledged queue, publication lead and recovery/horizon transition passes
independent journal checks. It publishes 548 warm backups and 31 short futures
from 41 short queries, with zero stale/rejected plans or follower error.
These different seeds do not establish a single-change improvement.

The same-profile repeat, seed 57265, also reaches the original terminal with
zero misses/bombs. Complete frozen replay matches all 47,986 native digests;
all retail projections and all 10,066 captured query transitions pass independent
checks. It publishes 1,091 warm backups and 91 short futures from 150 short
queries, with no stale/rejected plans or follower error. Its tape SHA-256 is
`3c9747c55cc71ad4cda96f6865e2de524bf8db2261915779c677e61cf1c96c76`.
The tested warm/rescue profile therefore has two consecutive natural-seed clears,
without attributing the combined change to any one feature or arbitrary seeds.

The first actual short query is at 25,347 with 78 queued inputs: it publishes an
80-update full warm checkpoint; the next query at 25,352 publishes a normal
144-update future. This demonstrates short-certificate adoption and expansion,
not a counterfactual proof that this particular query prevented a hit.

Full Present mean / slowest five seconds / p99 is 47.87 Hz / 21.6 Hz / 44.65 ms
for the failed warm trial and 45.83 Hz / 22.8 Hz / 47.37 ms for the clear.
Steady draw/Present-call means are 0.96/13.98 ms and 0.94/15.85 ms. Full-run
loading gaps remain; the pacing gate fails and no independent window clock is
claimed. The repeat reaches 40.51 Hz / 19.2 Hz / 54.13 ms, with steady
draw/Present-call means 0.97/20.09 ms, also a failing gate. Cumulative Extra accounting before the
subsequent runtime smokes is **three full clears, eleven full hits, two prefixes
and three zero-frame starts**. Random-seed robustness and stable
online 60 FPS remain open; published replay selections are unchanged.


The runtime follow-up adds two bounded actual online prefixes with the same MPC
policy and independent X11 window clocks. `online-runtime-default-004` uses the
new runner's default Wine 8 / win32 configuration, seed 23430, and stops at
1,200 updates. `online-runtime-dxvk-005` uses Wine 11 wow64 / the pinned private
DXVK early-flush DLLs and eight renderer threads, seed 14702, and stops at 3,600.
Both have zero misses/bombs, four startup fallback, complete independent prefix
replay and matching retail projections. Both intentionally end at the frame
budget (exit 4), never a clear. Private 64-bit Unix graphics libraries reach only
Wine; the native/display worker environment checks pass.

| Actual online prefix | Full API Present / window cadence | Steady draw / Present-call mean | Peak / OOM |
| --- | --- | --- | --- |
| Default, 1,200 updates | 51.88 / 51.96 Hz | 0.77 / 9.03 ms | 332.81 MiB / 0 |
| DXVK, 3,600 updates | 55.96 / 55.95 Hz | 1.30 / 4.68 ms | 351.63 MiB / 0 |

All resource caps are verified at 2 GiB / 1.5 GiB / no swap. The DXVK prefix's
window slowest five seconds / p99 is 43.0 Hz / 32 ms; both unchanged gates fail.
These different seeds, lengths and runtimes are functional observations, not a
paired speedup. Current cumulative Extra accounting is **three full clears,
eleven full hits, four prefixes and three zero-frame starts**. All raw inputs,
window/API clocks, runtime versions/hashes, effective environments and measured
whole-cgroup CPU totals remain private. The runtime/queue build passes 48 CTests.


Two separate **pure original-loader replay** diagnostics then reuse exactly the
3,600 actual inputs from seed 14702, with MPC and native follower both absent.
A private diagnostic builder checks every prefix digest and explicitly rejects
complete tapes; the public exporter's whole-clear requirement is unchanged.
All 3,600 supplied native projection rows match both playback traces. The
comparison's `complete_route` field covers the supplied prefix only: both retail
outcomes remain frame-budget stops with zero clears.

| Same-input replay control | Full API Present / window cadence | Window slowest 5 s / p99 | Peak / OOM |
| --- | --- | --- | --- |
| Hardware bullet path | 59.02 / 58.97 Hz | 54.0 Hz / 28 ms | 271.55 MiB / 0 |
| CPU bullet path with batching | 59.68 / 59.62 Hz | 57.2 Hz / 28 ms | 220.16 MiB / 0 |

Both retain actual 2-GiB/1.5-GiB/no-swap proofs and independent window clocks;
both unchanged pacing gates fail. The CPU path restores all 3,599 draw scopes.
Replay controls reuse warmed caches after the online run, and shared-host load
is uncontrolled. They do not measure a causal MPC slowdown or certify live FPS.
Persistent jitter without MPC rules out treating online search as the only
pacing cause in these observations. Batching remains replay-only; no live
rendering callback changes are promoted from this small comparison. Both pure
prefixes remain separate from the four actual online prefixes in the ledger.


## 2026-10-28 Wine 11 full-run counterexample

`online-runtime-full-008` keeps the tested warm/rescue MPC profile and Wine 11 /
DXVK hardware bullet rendering, with warmed caches and independent window timing.
Natural Extra seed **43546**, Reimu B, hits at **23,036**, spell 57 / ECL sub 44,
boss HP 1,264. Every native digest, retail projection and captured queue/policy
transition matches independent replay, including the hit. Tape SHA-256:
`67690e3f6a1b0eeebe915a029c1becddfee3b9726ee8b082b296c7d076abd122`.

There are 4,393 searches, 1,387 timeouts and 4,171 publications, including 563
warm backups and 14 short futures from 33 short searches. The last publication
at 22,954 is an 80-update warm checkpoint, ending at 23,034. Nineteen subsequent
replacements fail (15 timeouts), ending with two exhausted-queue inputs and the
hit. Total fallback is six including four startup; no rejected generation or
follower error occurs. One certificate is stale. Search p50/p99/max including
cleanup is 99.45/139.62/311.96 ms; actual deadline overruns remain reported.

The whole group has verified 2-GiB hard / 1.5-GiB high / no-swap limits, peak
404.38 MiB, OOM 0, measured CPU 886.91 s. Controller/children use 10.81/245.15 s
CPU over 505.94 s controller wall time. Full API/window means are 45.53/45.42 Hz;
window slowest five seconds / p99 is 21.8 Hz / 52 ms. Steady draw/Present-call
means are 1.20/16.02 ms. Both unchanged pacing gates fail. Different seeds,
profiles, cache states and host load prevent a causal renderer/algorithm claim.

Three source-replayed queue extractions capture exact initial planner state:

| Origin / remaining queue | Cursor / initial motion recovery | Ordinary sorting model | `diverse-first` model |
| --- | --- | --- | --- |
| 22,495 / 140 | 48 / off | Phase 57 ends 24,846, no hit/fallback | Same phase end, no hit/fallback |
| 22,684 / 139 | 56 / on | Hit 23,078, 6 fallback | Hit 23,739, 1 fallback |
| 22,803 / 139 | 21 / off | Phase ends 24,846, no hit/fallback | Hit 23,215, 1 fallback |

All six use full native futures, lead 16, 128/64 horizons, cap 110 ms, six
proposals and warm backups. The actual prefix supplies gameplay RNG; the exact
remaining literal queue is rebased at the target. Initial cursor/recovery and
previous replacement state are restored from the journal. Subsequent consumption
is still `max(1, ceil(search_ms * 0.06))` on this shared host. These phase episodes
are timing-dependent native experiments, not deterministic reproductions of the
actual varying Wine cadence or complete retail clears. Every whole trace passes
independent diagnostic digest replay, including all three hits; all surviving
prefixes also pass strict fresh replay. Their mixed outcomes do not justify
promoting diversity. All six attempts remain under private `captured-model/`.

This continuation retains **six actual online trials** (two clears, two hits,
two prefixes), **twenty native phase episodes**, the six-context scalar-cache
checks/benchmarks, and two separate original-loader diagnostic prefixes. The
cumulative actual Extra ledger is **three full clears, twelve full hits, four
prefixes and three zero-frame starts**. No new replay is published. Random-seed
robustness and stable online 60 FPS remain open; next work should target earlier
route choices and measured timing sensitivity in these captured spell-57 windows.


## 2026-10-28 independent warm horizon

Private evidence: `reports/local/extra-backup-20261028/`. A separately certified
64-update nominal backup can reserve more of the unchanged 110-ms cap for fresh
128/256-update futures. It never certifies the requested longer horizon. With
`--warm-horizon 64`, a backup covered by the old acknowledged queue leaves that
queue intact. Default zero retains the previous warm behavior; this option is
experimental, with no established random-seed or FPS benefit.

The seed-43546 spell-57 queues at 22,684 and 22,803 provide exact native prefixes,
remaining inputs, proposal cursors and initial recovery/replacement state.
Eight 128-horizon models alternate full/64 backup ordering over two repeats;
four more request 192. Four final 128-horizon models include the queue-preservation
guard and use the promoted implementation. All **sixteen phase episodes** survive
without hit/fallback and pass independent fresh replay. Their controls also
survive. One early short-backup phase ends at 24,651; the other episodes end at
24,846. Consumption is still `ceil(search_ms * 0.06)` on the shared host, not the
recorded varying retail cadence. GNU time retains CPU, elapsed time and maximum
single-process RSS; it is not a whole-tree memory measurement.

A full/64/64/full repeated-origin benchmark runs 30 fresh queries each at origin
22,803 with a 2,000-ms cap. All four fresh input/digest checksums match, with 240
branches each. Update counts are 31,200 / 29,280 / 29,280 / 31,200. Child CPU is
3.506 / 3.500 / 3.459 / 3.356 s: fewer updates do not establish a CPU reduction.
The first benchmark may overlap fresh-check work, so no timing speedup is claimed.
All four trials are retained. The final code passes **51 CTests** with real DATs,
including short-proof length, failed long futures, deadline cleanup, native
literal replay, fresh priority and preserving longer acknowledged queues.


## 2026-10-28 short-backup retail failure and search cost

The first actual `--warm-horizon 64` trial, `online-short-warm-001` under private
`reports/local/extra-backup-20261028/`, captures Reimu B seed **24944**, natural
power-0 Extra. It hits at **34,033**, spell 62 / sub 60, boss HP 1,000. All native
digests and retail projections match independent replay including the hit;
all 7,004 queries and 6,557 literal publications pass the updated journal graph.
The final publication at 33,951 certifies 80 updates (lead 16 + backup 64),
correctly recording requested horizon 256 separately. It ends at 34,031; thirteen
unsuccessful replacements (nine timeouts) follow, then two fallback inputs and
the hit. Total fallback is six including four startup. No stale/rejected plan
or native follower error occurs.

The run retains 1,021 timeouts, 431 covered backups, 16 warm publications and
15 short-warm publications. Verified limits are 2-GiB hard / 1.5-GiB high /
no swap, peak **609.82 MiB**, OOM 0. Controller/children use 15.78/471.54 s CPU
over 720.61 s controller wall time. Actual window mean / slowest five seconds /
p99 is **47.22 Hz / 20.2 Hz / 45 ms**, with steady draw/Present-call means
0.95/14.43 ms. Both unchanged gates fail. This one different-seed failure neither
proves a regression from the backup cap nor establishes benefit; the cap remains
experimental and is not recommended from this result.

Six subsequent native phase models restore exact queues and planner state with
256-tail/110-ms/six-proposal policy:

| Origin / queue / cursor (recovery off) | Full backup | Short backup with queue protection |
| --- | --- | --- |
| 33,003 / 265 / 22 | Hit 33,792; 1 fallback | Hit 34,035; 1 fallback |
| 33,402 / 267 / 24 | Phase ends 37,762; no hit/fallback | Same phase end; no hit/fallback |
| 33,750 / 265 / 42 | Phase ends 37,762; no hit/fallback | Same phase end; no hit/fallback |

All six whole traces independently replay, including hits. A second six-episode
batch compares the unchanged full-backup policy with privately reordered
reactive/orbital candidates first during invulnerable survival phases. At the
first origin, the control survives while reactive-first hits at 34,043 with
11 fallback; both survive the middle origin; both hit at the last origin
(control 34,046 / 5 fallback, variant 34,319 / 3 fallback). All six also replay
independently. The identical baseline binary, initial queue and planner state
can change outcome between these batches: `ceil(search_ms * 0.06)` consumption
and deadline truncation remain host-timing dependent. These comparisons cannot
establish causal retail recovery. Reactive-first is not promoted.

A checked snapshot at 33,750 has player (267.549, 432), 386 visible bullets,
no lasers, and bullet flags 0x202 (spawn effect/sound, no turning/acceleration
mode). Current linear forecasts omit future spawns. A native 512-tail probe
finds a multi-turn reactive path across the old hit: **528 certified updates**,
30 branches, 4,618 updates, **505.95 ms**. Privately prioritizing reactive paths
finds a full future in 13 branches / 2,922 updates / **385.52 ms**. Both complete
prefix-plus-future tapes pass frozen fresh replay, but both 110-ms probes time
out with no certificate. Feasibility with a 2,000-ms cap is not online feasibility.
A private vertical-margin score instead exhausts 50 candidates / 6,780 updates
in 832.54 ms without a complete 512-tail future; it is not promoted either.

This continuation retains **one failed actual full trial, 28 native phase
episodes and five fixed-prefix probes**, including every negative outcome.
Native model GNU-time CPU totals are 1,005.70 s across 11,048 queries; maximum
RSS remains per process rather than a tree peak. The cumulative actual Extra
ledger is **three full clears, thirteen full hits, four prefixes and three
zero-frame starts**. No new replay or asset is tracked. Random-seed robustness
and stable online 60 FPS remain open. Next work should separate repeatable
candidate-quality checks from captured-latency policy episodes and reduce the
cost of useful multi-turn proposals, rather than raising the online cap.
