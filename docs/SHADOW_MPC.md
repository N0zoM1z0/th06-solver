# Live Wine shadow MPC

The controller now drives the owned Japanese 1.02h retail process under Wine.
Two complete Lunatic Stage 5 / Reimu B runs and one Stage 6 / Marisa B run pass
NMNB, including a Stage 5 live recording. All three actual input tapes pass
independent complete native replay. Five failed Stage 6 controls remain in the
evidence; this is not all-seed, all-stage or full-story retail certification.

## Run

Build the native tools normally and enable the optional MinGW bridge:

```sh
cmake -S . -B build -DTH06_WINE_REPLAY=ON
cmake --build build -j 2
python3 scripts/run_wine_shadow.py \
  --game /path/to/private-game-copy --prefix /path/to/dedicated-wine-prefix \
  --data /path/to/same-six-dat-archives --stage 5 --shot 1 \
  --horizon 128 --lead 16 --budget-ms 110 --interval 16 \
  --updates 40000 --seconds 900 --output reports/local/live-stage5

# Faster feedback profile used for the Stage 6 component clear.
python3 scripts/run_wine_shadow.py \
  --game /path/to/private-game-copy --prefix /path/to/dedicated-wine-prefix \
  --data /path/to/same-six-dat-archives --stage 6 --shot 3 \
  --horizon 64 --lead 8 --budget-ms 80 --interval 4 --proposals 3 --fresh \
  --updates 40000 --seconds 900 --output reports/local/live-stage6
```

Difficulty is currently fixed to Lunatic. A standalone start uses the original
practice initialization: power 0/64/128 for stages 1/2/3–6. `--story --stage 1`
requests a natural story start; a frame-limited handoff check is not a complete
story. The EXE and dedicated 32-bit Wine prefix requirements are the same as
[retail replay](RETAIL_REPLAY.md). The runner checks the pinned EXE and equality
of all six DAT files, snapshots executable/controller/DLL identities, locks the
owned prefix, freezes the Python runner/capture modules as well, and places
Wine, Xvfb, controller, descendants and optional capture
processes in one 2-GiB/no-swap user-systemd cgroup. It never attaches to an
existing game or changes the disk EXE.

## Boundary contract

This follows th08-solver's common-start, true-RNG shadow architecture, rather
than importing pointer-rich managers from Wine. Each native branch owns its
world, scripts, actor pointers and actual action-dependent RNG through `fork`.
Retail owns collision, RNG, rendering, clocks, recording and natural lifecycle.

| Crossing | Contract and authority |
|---|---|
| Retail initialization → native start | Original game initializes first and publishes stage, difficulty, shot, pre-stage-init RNG, power and story mode. Native independently initializes the same case. |
| Retail calculation → native follower | A versioned 20-word journal record identifies every actual input and post-update projection. Partial writes wait; missing/repeated frames fail. Native executes actual receipts in order. |
| Actual world → speculative child | Immutable queued/fallback inputs form the lead. Only a complete native future or real terminal supplies a new queue; failed futures supply no prefix. |
| Child → controller | Whole-process-group wall budget includes lead, proposals, native verification, hashing and IPC. Timeout kills and reaps descendants. Interrupted work cannot publish. |
| Controller → retail input sampler | Double-buffered aligned atomic words carry a monotonic generation, origin, stage, first change and literal inputs. Retail copies an accepted plan privately and rejects stale results, wrong-stage plans and changed queued lead. |
| Natural stage handoff | Retail resets its old queue. Native mirrors that change after actual receipts and follows source RNG/resources without reseeding. |

The input callback runs at source calc priority 5, before stage/player updates;
the observer runs at 15, after gameplay. The non-replay retail recorder remains
active. No whole-frame waits for the controller are inserted into the game.

The declared projection compares frame/input, stage/timeline, player x/y, RNG
seed/draw count, power, deaths/bombs, score/rank, player state, cleared stages and
terminal meaning. Positions permit 0.025 game units of drift; other projected
values are discrete and exact. These scalars detect relevant divergence but do
not serialize or prove equality of the hidden world. Native branch/parent and
fresh native replay still compare exact per-update observable digests.

## Scheduling and costs

The initial profile preserves 16 lead inputs, then verifies 128 newly generated
updates: a successful nonterminal plan contains 144 literal inputs. The `horizon`
option counts the new tail, separately from the lead. Version-2 transport holds
at most 600 inputs, so a 512 tail with lead 40 verifies/publishes all 552 updates.
The horizon is 32–592, lead 8–64, with their sum bounded by 600. Native and Wine
must use matching bridge versions. Archived version-1 runs retain frozen binaries.
Native tape format is unchanged. The larger child certificate travels in one
`SOCK_SEQPACKET` message, so a readable partial pipe cannot bypass the deadline.

Every 16 actual updates, a search first tries to extend the existing literal
queue on three out of four attempts; the fourth requests a fresh proposal.
Fresh candidates use the existing native planner with one required successful
future and reactive tails. `--proposals N` raises the minimum evaluated candidate
count before the planner can stop with a safe winner; `--fresh` skips nominal
extension and requests fresh candidates at every interval. Defaults remain
128/16/110/16 with one candidate and nominal reuse. The minimum interval does not
imply a 16.67-ms search deadline. A 110-ms total search cap fits inside the 16-update lead;
fork/termination/reaping can finish slightly later and actual elapsed time is
reported. After search, the controller catches up to actual receipts and checks
every consumed predicted input/digest before publishing. Retail checks freshness
again at input sampling. A depleted queue explicitly holds the last movement and
focus while shooting/skipping; fallback is not a certified rescue.

`--pulse` is a separate optional experiment adapted from TH08's native motion
recovery: 18 held direction/focus actions and 48 moves lasting 16/32/64 updates
followed by a stationary tail. After nominal failure, they precede heuristic
proposals. An attempted-candidate cursor survives timeouts and advances across
actual origins; the winner receives independent digest-producing verification.
Its first Stage 6 sample also fails, so it is not the default or an established
robustness improvement.

Online search can defer digest calculation on candidates after the first. A
later winner receives an independent native literal replay before `choose`
returns its complete digest certificate. Verification branches, updates and
timings are charged. The first successful candidate retains its original fast
path. Offline defaults and its action selection remain unchanged.

Reports include every search wall time, timeouts, publications, stale results,
nominal reuse, actual planned/fallback updates and receipt spacing. Completed
search branch/update counters explicitly exclude interrupted unknown work; they
are not total search-volume claims. Controller/child CPU and the whole cgroup's
resource receipt account for time spent on interrupted work separately.

## Actual retail evidence

Cases in this table use Lunatic and the initial 128-tail/16-lead/110-ms profile. Seeds below
are captured from the real retail initialization, not chosen after a solve.

| Case | Retail result | Actual updates | Searches / timeouts | Actual fallback |
|---|---|---:|---:|---:|
| Stage 5, Reimu B, seed 51280 | Complete NMNB | 22,360 | 1,397 / 53 | 16 startup |
| Stage 5, Reimu B, seed 42686, recording | Complete NMNB | 22,317 | 1,394 / 30 | 16 startup |
| Stage 6, Marisa B, seed 47489 | Hit, incomplete | 14,230 | 889 / 51 | 16 startup + 6 exhausted |

Two subsequent, independent-seed Stage 6 experiments also fail: 240 tail /
16 lead at seed 32558 hits at update 5,200 (324 searches, 39 timeouts), and the
same horizon with `--pulse` at seed 56175 hits at 3,617 (226 searches, 55 timeouts,
1,041 motion attempts including interrupted work, 54 motion publications).
Every retained projected row agrees with native, including the actual hit.
These different-seed trials are not paired proof that one policy is better.

A separate natural-story handoff check reaches Stage 2 at the fixed 20,000-update
budget, with one natural clear, zero misses/bombs and full fresh native replay
of that entire prefix. It has 1,249 searches, five timeouts, 22 fallback inputs
(16 startup plus six at the stage handoff), and a 392,019,968-byte cgroup peak.
Its search median/p99/max is 10.36/69.94/112.54 ms. This verifies the first
lifecycle boundary and remains an incomplete story, not a six-stage clear.

Both Stage 5 actual tapes pass independent complete native replay. The first
run's search median/p99/max is 15.25/111.49/112.82 ms; recording run is
13.36/111.21/112.09 ms. Timeouts retain the prior safe queue. Neither Stage 5
run has owner rejections or stale publications. Cgroup peaks are 353,722,368
bytes without recording and 821,743,616 bytes with recording/muxing; OOM and
OOM-kill counters are zero. These are finite live samples, not all-seed guarantees.

The initial Stage 6 seed remains a counterexample. Its final accepted plan
originates at 14,080; subsequent searches time out through 14,208, then the lead
itself fails at 14,224.
At origin 14,112, an unbounded diagnostic evaluates all 50 proposal futures and
reaches only 101 safe tail updates. Additional CPU alone does not make that
proposal family certify 128. Earlier, at origin 13,952 plus the actual 16-input
lead, a 240-update native future is found in about 72 ms. At origin 14,016 the
short profile finds 128 safe updates, while all longer diagnostic candidates
reach at most 172. These checked-prefix diagnostics motivate testing longer
verification before this trap; they do not prove a counterfactual retail clear.

Further Stage 6 component runs retain both failures and the new success:

| Seed | Tail / lead / cap / interval | Proposal policy | Retail outcome | Searches / timeouts | Fallback |
|---|---|---|---|---:|---:|
| 57766 | 128 / 16 / 110 ms / 16 | Recorded, one endpoint per refinement | Hit at 5,174 | 323 / 62 | 24 |
| 18751 | 512 / 40 / 480 ms / 32 | Nominal plus native planner | Hit at 5,002 | 156 / 8 | 35 |
| 10347 | 64 / 8 / 80 ms / 4 | Fresh, minimum three candidates | Complete NMNB at 28,254 | 7,050 / 63 | 4 startup |

The recorded run publishes 75 independently native-verified geometry proposals.
Its timeouts include 31 during geometry search, 18 during native verification and
13 in the ordinary planner. Geometry proposals are usable, but this run still
exhausts the queue. The long-buffer run has 17 completed unsuccessful searches;
its final searches finish without a certificate in 246/232 ms, below the 480-ms
cap. Increasing the deadline alone cannot fix that candidate-family limitation.

The successful fresh profile has median/p99/max search time 29.07/77.70/82.87 ms,
6,985 publications, zero stale results and zero retail rejections. Every input
after the first four uses a verified plan. Whole-cgroup peak is 354,058,240 bytes;
OOM/OOM-kill counters are zero. Controller/child CPU is 24.31/201.69 seconds over
483.43 seconds of controller wall time. Completed searches execute 26,255 native
branches and 1,614,007 updates; interrupted work is excluded from those counters.
This is a real component clear with faster feedback, not a paired demonstration
of improved robustness or lower CPU.

A separate native timing model uses seed 47489 and advances the actual source
world by `max(1, ceil(search_ms * 60 / 1000))` updates during each search. It checks
every consumed predicted input/digest and preserves the lead. Initial,
fresh-three-candidate and fresh-six-candidate profiles all fail at
14,505/6,788/12,181 updates respectively. This models execution lag without Wine
or rendering, so it is diagnostic evidence only. Different-seed retail trials
and this model cannot establish universal policy improvement. Complete live
story control and broader seed robustness remain open.

## Conditional geometry experiment

`--recorded` is separate from video capture and remains optional, with tail at
most 240. Read-only headless hooks capture source collision arguments in a native
fork: lethal bullet/body AABBs, raw rotated-laser rectangles and movement context.
Both `CheckGraze` and `CheckCollision` are needed: distant ungrazed bullets can
skip the latter; the former must be recorded before its graze-size expansion.
Laser proposals use TH06's clockwise rotation and original startup/despawn
arguments. Movement records source time-stop, vulnerability and speed multipliers.
All source predicates, callbacks, RNG and x87 gameplay arithmetic are unchanged.

A 64-node spatial beam tries multi-turn paths with two-update near controls and
four-update distant controls, preserving coarse spatial diversity before filling
by score. Only this proposal translation unit uses SSE2 arithmetic. Up to four
endpoint-diverse paths receive full native checks before another geometry search;
the deepest failure supplies conditional geometry for the next refinement.
At most three refinements use the union of recorded constraints. A safe candidate
then receives a second independent native replay producing every literal digest.
Unrecorded futures and approximate geometry never certify a path.

Five shared tape slots contain at most 240 frames, each with 2,048 boxes and 512
lasers (about 54 MiB of virtual raw storage). The combined spatial-index entries
across all frames/refinements are capped at 32 MiB before allocation. Capacity or
nonfinite data fails explicitly. The parent deadline covers capture, index
construction, beam search, all verification descendants and the fallback planner;
phase/forecast/check counters survive a killed child. The full old Stage 5 video
tape still replays with identical native digests after adding these passive hooks.

At the checked late origins 14,080/14,112 of seed 47489, the current four-path
portfolio still times out under 110 ms after four native checks. It does not
rescue that known failure window. The feature remains an instrumented proposal
experiment, off by default.

## Live recording

Add `--record` with `ffmpeg`, `xdotool`, PulseAudio and its utilities installed.
Use a private recording game copy with WAV music and sound enabled and its
original BGM files present. The main testing copy's configuration can remain
independent. Configuration hashes and the actual seed accompany the recording.

As in th08-solver, capture the owned game's 640×480 rectangle on the Xvfb root;
startup may recreate the window ID. Video and audio use separate processes, a
single libx264 capture thread and a private PulseAudio null sink/monitor. Original
device clock origins align 48-kHz PCM before AAC muxing. Pad a shorter audio tail
instead of truncating captured game frames. Matroska retains original video;
delivery compression keeps actual timestamps without interpolation or changing
game speed. An explicit microsecond encoder time base avoids rounding nearby
capture samples to the same nominal-60-FPS timestamp.

The complete Stage 5 recording contains 22,602 video frames over about 377 seconds,
with no duplicate timestamps, p99 interval 17 ms and maximum interval 32 ms.
The compressed 640×480 H.264/AAC delivery retains every frame, passes full decode,
and includes audible game music/effects. Capture cadence does not itself prove
60-Hz gameplay. All movies, audio, raw journals, failed attempts and native tapes
remain under ignored `reports/local/`.

Tests cover torn map writes, stale/changed leads, stage identity, input bounds,
float tolerance, literal native prediction/COW isolation, failed futures,
maximum-size 600-input plans, independent deferred-hash and recorded winner
verification, raw collision geometry, explicit index-capacity rejection, and
repeated forced deadlines in nested source rollout descendants with no surviving
children or zombies. Twenty-three focused contracts pass; full matrices and
other-difficulty story tests were not rerun for this optimization.
