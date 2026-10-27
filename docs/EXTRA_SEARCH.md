# Extra online search optimization

The 2026-10-27 experiment targets the actual failed Extra Reimu A routes, not a
new offline matrix. Historical seed-0 power-128 checkpoints and normal power-0
Wine starts remain separate. Full Extra online NMNB and stable online 60 FPS
are not established.

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

Thirty-one CTest contracts pass, including actual original BulletManager
collision-call/lifecycle comparisons and literal replay with guidance history
enabled. All four historical Extra tapes still pass complete fresh native replay.
The source and its licenses/provenance are unchanged.

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

One unimplemented direction is to expose an independently replayed, full-horizon
candidate while further ranking continues, so a later timeout cannot discard an
already certified result. Publication, digest production and process cleanup
must all fit the measured budget. Another is continuation-aware route ranking
before the edge trap; late emergency motion alone has not resolved this case.
