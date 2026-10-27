# Native runtime status

Current owner scope (2026-10-27): Extra online shadow MPC with Reimu A,
original retail power-0 entry and captured retail seeds. One incomplete
2,400-update prefix passes; eight completed full trials hit. **Complete Extra
online NMNB and stable online 60 FPS remain unestablished.** See the
[full live ledger](SHADOW_MPC.md) and [focused search study](EXTRA_SEARCH.md).

The optional search refinements preserve complete native verification:

- `--laser-aware`: shared source-based laser forecasts and literal orbital routes.
- `--anytime`: immutable first-certificate recovery after deadline cleanup, plus
  a 75% ranking cutoff once a full future exists, reserving verification time.
- `--power-aware`: original native power-item outcomes guide proposals until 128.
- `--survival-horizon 256`: longer futures only for active non-interactable spells.
- `--damage-aware`: actual native boss HP and phase completion guide ranking.
- `--acceleration-aware`: observed finite acceleration guides bullet proposals.

Forty local CTest contracts pass. Two promoted pickup opening models
reach full power and pass frozen replay. Survival-only lookahead passes one
captured spell-62 model but fails the older seed. Damage-aware 128-tail models
end spell 58 in two captured windows and fail a third; all successful prefixes
pass frozen replay. Global 256-tail spell-58 models all fail under the same
110-ms cap, so that profile is not adopted. These are native timing models,
not retail clears or paired all-seed robustness claims.

The latest completed Wine trial, seed 5594, uses damage guidance and reaches
full power, passes spell 55, then hits at update 22,062 on spell 57 / ECL
subroutine 44, boss HP 946. Every retail projection and independent actual native
digest agrees, including the hit; strict NMNB replay rejects it. Its 4,921
searches include 81 timeouts, 18 published deadline certificates, 47 ranking
yields and eight fallback inputs (four startup). Search p99/max is
111.08/112.51 ms including cleanup. Group peak is 398.15 MiB, OOM 0 under
verified 2-GiB/no-swap limits; all 22,181 limiter scopes restore.

Source inspection of that capture finds dozens of accelerating bullets omitted
from constant-velocity proposal forecasts. The optional acceleration snapshot
respects original expiry, update order and competing-mode precedence. Actual
BulletManager callback tests pass. Two captured-window models and their controls
all end the spell without hits/fallback; one guided route is faster and one is
slower. All four prefixes pass frozen replay. No general robustness improvement
or complete retail clear follows from these model results.

Three zero-frame startup attempts are retained separately. Early scoped
limiter precision fixes their demonstrated long-uptime subtraction hang;
optional coarse sleeps retain the original clocks and checks. This does not
establish smooth online presentation. All failed experiments/retries remain
under private `reports/local/shadow-extra-20261027/`,
`reports/local/extra-search-20261027/` and
`reports/local/extra-online-20261027/`.

Previous pacing scope (owner clarification, 2026-10-26):
[native-generated `.rpy` → actual Wine replay → stable 60-FPS presentation](REPLAY_PACING.md),
with no MPC or native follower during playback. Online control acceptance stays
separate. Native re-export revalidates the historical 127,737-update Lunatic
Reimu A story and reproduces the published replay hash.
The current profile passes one complete Lunatic Reimu A seed-0 story: 127,737
native-matching updates, six natural clears and zero misses/bombs. API output is
59.990 Hz / slowest 5 s 58.6 Hz / p99 17.130 ms; independent actual window output
is 59.990 Hz / slowest 5 s 58.4 Hz / p99 18 ms. Both unchanged version-4 gates pass.
The profile combines Wine 11 new WoW64, 64-bit Mesa 25 lavapipe, source CPU/diffuse
bullet drawing, bounded batches, coarse sleeps, mapped journals, eight workers
and an opt-in DXVK 2.4 D3D8 EndScene submission patch. Peak is 490,835,968 bytes
(468.1 MiB), OOM/OOM-kill 0 under verified 2-GiB/no-swap limits. Whole-group CPU is
3,993.08 seconds, 26.7% below the retained earlier complete-story profile.
The earlier 56.4-Hz Stage 4 window now reaches 59.4 Hz, p99 18 ms. All handoff
gaps remain; the largest window interval is 145 ms. That loading spike remains
an optimization target. This one-case replay proof does not establish online
MPC FPS or all-seed performance. Package/DLL hashes and settings are pinned in
[REPLAY_RUNTIME.json](REPLAY_RUNTIME.json).

Separate 2026-10-27 recording: the published replay's exact SHA-256 again reaches
all six natural NMNB clears in Wine, with all 127,737 updates matching native.
The new `run_wine_replay.py --compact` captures the full 640×480 rectangle and
private original game audio, then compresses after Wine exits. With WAV music
and effects enabled, its handoff gaps are longer. This recording does not pass the
unchanged presentation gate or replace the earlier audio-off pacing acceptance.
The complete compact MP4 is 35:35.744 / 503,859,305 bytes. All 128,141 frames decode
with unchanged capture PTS and audible AAC audio; group peak is 1.501 GiB, OOM 0.
See [recording instructions](REPLAY_PACING.md#record-the-replay).

For the 2026-10-26 restart point, prioritized unfinished tasks, implementation
map and local evidence index, see [CONTINUATION.md](CONTINUATION.md).
The active checklist is [issue #1](https://github.com/N0zoM1z0/th06-solver/issues/1).
**Online shadow MPC does not yet sustain stable 60 FPS.** Its live stage clears
establish gameplay progress; frame pacing remains a separate high-priority task.

Separate live milestone: [Lunatic shadow MPC in Wine](SHADOW_MPC.md), with bounded
decision time and actual input/queue accounting. Two complete Stage 5 / Reimu B
runs and one Stage 6 / Marisa B run pass NMNB, including a Stage 5 live recording
and independent complete native replay of all three actual routes. Stage 6 seed
10347 clears in 28,254 updates with a 64-tail/8-lead/80-ms profile, fresh proposals
at a minimum four-update interval and a three-candidate minimum. Search median/p99/max is
29.07/77.70/82.87 ms including cleanup; only four startup inputs use fallback.
The whole Wine/controller cgroup peaks at 354,058,240 bytes with zero OOM events.

Five other actual Stage 6 runs fail, including recorded-geometry and long-buffer
controls. A separate same-seed native timing model also fails under three
policies; the new clear does not establish a general policy improvement.
Optional collision-phase geometry proposals require independent native
verification; their storage and spatial index have explicit bounds. Version-2
transport carries up to 600 literal inputs with an atomic child certificate.
Twenty-three focused contracts pass. Natural handoff and all live experiments
remain separate from the older complete offline and retail-replay results below.
Current experiments focus on [Lunatic hard-stage performance](PERFORMANCE.md),
not another full matrix or story campaign. The latest 20 new paired stage-5/6
cases compare `f942133` with opt-in native-future retention (`--reuse 32`). Both
versions clear 20/20 after the same recovery policy, including all failures and
retries. Speculative native updates fall from 39,991,255 to 11,297,133 (−71.75%)
and planner CPU from 2,659.12 to 935.33 seconds (−64.83%). Initial clears fall
from 18/20 to 16/20, so the longer publication window remains optional.
Eleven focused contracts pass, and all 40 final tapes pass fresh replay under
both the frozen baseline checker and current build. Earlier kernel optimizations
retain their separate 1.5–2× unchanged-input timing result; diverse-first ordering
is off in this new comparison.

The earlier [Lunatic random-seed campaign](LUNATIC_RANDOM_SEEDS.md) remains a
separate historical control.
The four fixed random seeds pass 96/96 standalone cases and 16/16 continuous
stories after checked-prefix recovery (92/96 and 11/16 initially). Every route
passes full fresh native replay with zero misses/bombs. These 112 sampled cases
contain 4,057,594 accepted updates; peak campaign memory is 140,361,728 bytes
(133.86 MiB), with zero OOM events under the 2-GiB/no-swap cap. Random-seed results
are native evidence for this fixed sample; see the separate experiment ledger.
The all-difficulty seed-0 results below remain historical controls.
The complete standalone matrix passes 96/96 and continuous stories pass 16/16,
including fresh native replay of every entire route. The owner has now authorized
Wine retail replay interoperability. The complete Lunatic Reimu A seed-0 story
now also passes retail playback: six natural clears, zero misses/bombs and all
127,737 rows matching the declared 15-field native projection. Its full-playback
cgroup peak is 348,819,456 bytes with zero OOM events under a 2-GiB hard limit.
That one retail case is separate from the 112-case native matrix; details are
recorded in [Retail replay](RETAIL_REPLAY.md).

## Acceptance matrix

| Suite | Domain | Complete fresh-replayed NMNB cases |
|---|---|---:|
| Easy | Stages 1–5, four shots | 20/20 |
| Normal | Stages 1–6, four shots | 24/24 |
| Hard | Stages 1–6, four shots | 24/24 |
| Lunatic | Stages 1–6, four shots | 24/24 |
| Extra | Stage 7, four shots | 4/4 |
| Continuous story | Four standard difficulties × four shots | 16/16 |

Easy story ends after stage 5 as in the source; the other stories end after stage 6.
An earlier terminal event is rejected, even if the current route has no misses.

The initial seed is 0. Standalone checkpoints use power 0 at stage 1, 64 at
stage 2, and 128 at stages 3–7. Story starts at power 0 and carries actual pickups.
Additional seed coverage is separate from the matrix above.

A case passes only on the native terminal transition with zero misses/bombs and
a matching complete replay in a fresh process. A planned prefix, timeout, runtime
error or failed replay does not pass. All logs/tapes/reports stay in reports/local/.

## Verified evidence

Development environment: Linux x86-64, Clang 14.0.6, native 32-bit build, no
fast-math. The original EXE SHA-256 is
`9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245`.

- All six DAT archives parse; all members decompress and pass stored checksums.
  There are 337 distinct resource names; identical shared version members are
  accepted and conflicting duplicate content is rejected.
- First baseline: stage 1, Lunatic, Reimu A, seed 0, power 0. Fixed shoot/focus/skip
  input dies at update 319. Two fresh processes produce identical complete traces.
- First full solve: the same configuration completes at update 13,701 with zero
  misses/bombs. The exhaustive 33-proposal planner used horizon 90, commit 8,
  56,529 native branches and 4,355,906 simulated updates in 218.40 seconds.
  A new checker process replays every accepted update and confirms completion.
  The local tape SHA-256 is
  `4e575d68add3a8d412275791456877218fa0cc5df99da49c9f870cbe8b374138`.
- Initialization and live collisions have been exercised in stages 1–7 using
  Marisa B, seed 65535 and power 128. These short negative controls are not clears.
- Tests cover PBG3 truncation/checksums/overlapping dictionary references, tape
  sequencing and corruption, source RNG ordering, collision edges, unsupported
  opcodes and a planned prefix with fresh-process replay and rejection controls.

The complete 96-case standalone matrix uses the canonical checkpoints above.
Earlier standalone stage-2 power-128 experiments are separate evidence; the
matrix stage-2 checkpoint uses power 64. A ranked four-proposal run reproduces
the stage-1 completion in 26.80 seconds with 6,967 native branches and 603,011
simulated updates. All retry effort remains in the local attempt ledger.

Nine focused tests pass, including a source-binary replacement regression for
campaign snapshots, RNG/resource carry across a stage handoff, atomic rejection
of invalid handoff input, Extra's source bootstrap/score-extend rule, reactive
future continuation replay, and rejection of a premature story terminal. A clean Release
build passes the asset-free contracts and replays the initial complete tape.
The final standalone gate independently replays all 96 complete tapes under the
current runtime and reports 96 cleared, 0 failed. Earlier incomplete searches and
their costs remain in the attempt ledger.

Extra Reimu B now completes at update 59,837 with zero misses/bombs and passes
fresh replay under the corrected runtime. Verified-prefix rewind first crossed
the update-13,078 dead end with horizon 180/proposals 8, then the update-47,943
dead end with horizon 240. Source review found the omitted Extra score-extend
disable; the corrected bootstrap selects source score-table index 4. Its first
observable difference occurred at update 58,556, so the route was resumed from
the checked prefix ending at 58,555 and fully replayed again. All earlier attempts
and their simulation/replay costs remain recorded; the last continuation's timing
is not the cost of the entire solve. Reactive native futures then complete the
remaining Extra shots. Their current complete-route evidence is:

| Shot | Complete native updates | Last continuation horizon / proposals | Reactive futures |
|---|---:|---|---|
| Reimu A | 47,240 | 240 / 8 | Enabled |
| Reimu B | 59,837 | 240 / 8 | Disabled |
| Marisa A | 55,702 | 240 / 8 | Enabled |
| Marisa B | 61,696 | 320 / 12 | Enabled |

Every entry has a complete new-process replay with zero misses/bombs. The last
continuation budgets above do not replace earlier checked prefixes or erase retry
costs.

Continuous stories start at stage 1, power 0, and carry native state through every
natural stage transition. Complete update counts below have matching new-process
replays, zero misses/bombs and all required stage clears:

| Difficulty | Reimu A | Reimu B | Marisa A | Marisa B |
|---|---:|---:|---:|---:|
| Easy | 86,336 | 89,323 | 87,424 | 91,853 |
| Normal | 117,897 | 119,018 | 117,072 | 124,054 |
| Hard | 119,829 | 124,260 | 123,471 | 129,796 |
| Lunatic | 127,737 | 133,648 | 130,563 | 141,393 |

These story searches use reactive futures, horizon 120, commit 8 and six ranked
proposals, with a 180,000-update frame budget per complete story. The budget is
not a completion criterion. All 16 complete stories were planned from their
natural starts without prefix recovery in this campaign. Together they contain
1,863,674 accepted and independently replayed updates.

The final local evidence receipt cross-checks all 112 case identities, start
profiles, completion flags, zero-miss/bomb counts, required stage counts and
planner/checker/tape frame counts. It records tape/report SHA-256 identities and
confirms the frozen story binaries match the current build. The complete
standalone and story routes contain 3,854,742 accepted updates in total; this is
route length, not total search effort. The receipt is stored at
`reports/local/native-acceptance-final.json`; tapes and failed-attempt evidence
remain private and ignored.

Each campaign snapshots its planner/checker binaries and records their identities.
Recompiling the workspace cannot change a running campaign. Resume rechecks each
existing completed tape in a new process before counting it. Startup failures from
the initial unsnapshotted run remain recorded as infrastructure failures, not
gameplay counterexamples or silently discarded attempts.
