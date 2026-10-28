# Continue online shadow MPC work

Read [engineering rules](../AGENTS.md) and [current status](STATUS.md) first.
The current gameplay target is Extra online robustness and decision/presentation
performance. Three normal-start Extra Reimu B seeds clear NMNB with complete
fresh native replay; the original sample also has verified retail replay and
recording. Full-story online coverage and stable online 60 FPS remain open.
Detailed historical Lunatic investigations are preserved in [Online history](ONLINE_HISTORY.md), not a second current plan.

## Restart checklist

1. Inspect the working tree and the selected case's start, frozen binaries,
   receipts, inputs and resource proof. Keep actual online, native model and
   original-loader playback evidence distinct.
2. Use [Validation](VALIDATION.md#build-and-focused-tests) for the configured
   local gate and affected real-DAT tests. Recheck any changed actual route in
   a fresh process with `--require-complete`; a prefix stays incomplete.
3. Select a few captured hard windows before spending on complete Wine attempts.
   [Extra search](EXTRA_SEARCH.md) retains both model successes and failures;
   [Shadow MPC](SHADOW_MPC.md#run) gives actual run commands and constraints.
4. Preserve every retry and total effort. Publish tested code and compact English
   documentation on main. Keep assets, tapes, videos and raw reports private;
   only the two owner-selected [input-only replays](../replays/README.md) are tracked.

One Wine instance, normally 2 GiB/no swap; the recorded explicit 4-GiB allowance
applies to replay recording. Use at most four solver jobs. Private local helper
sources are evidence, not supported tools that exist in a fresh checkout.

## P0: Make failed windows repeatable and measure queue risk

Extra failures include spells 57, 58 and 62. The retained Stage 6 seed-47489
right-edge failure is another useful older control. Start before the trap:
more time after every candidate fails does not create a continuation.
The newest Extra seed-4902 spell-62 capture has checked 38,100/38,500 windows;
its actual queue ends at 38,782 and the hit is at 38,787. A 256/110-ms native
latency model ends the phase, while 256/200 ms hits and 320/200 ms incurs 77
fallback inputs. These are retained counterexamples, not retail recoveries.

`th06_shadow_probe` now takes an actual tape, origin, immutable lead and search
profile. It checks every prefix/lead digest and literally replays each returned
certificate; its output remains a fixed-prefix native experiment. Freeze the
binary and arguments with the experiment. See [Shadow MPC](SHADOW_MPC.md#checked-prefix-probes).
Separate fixed-prefix comparisons from closed policy episodes and from actual
retail trials. Native models using `max(1, ceil(search_ms * 60 / 1000))` consumed
updates are timing-dependent: host load can change the route and action RNG.

Record queue remaining, certificate age, consumed lead, adoption lag, timeout
phase, cleanup cost and fallback reason. Completed branch/update totals exclude
unknown interrupted work; controller/child/cgroup CPU still accounts for it.
A held input after queue exhaustion is fallback, never a certified rescue.

Acceptance: reconstruct one retained window, validate its exact prefix and
separate timeout, completed no-certificate, stale rejection, stage reset and
queue exhaustion. Blocked descendants must still be killed/reaped without
mutating the parent world. Retain all negative outcomes.

The newest verified clear is seed 57265, `online-rescue-003`, following seed
61515, `online-rescue-002`, under private
`reports/local/extra-control-20261028/`, using full native warm backups and
`--rescue-horizon 64` with the original 128/256 horizons and 110-ms cap.
The journal captures every published future, acknowledged generation and proposal
cursor. Use `scripts/shadow_queue.py` at an exact query origin to reconstruct
and independently verify the actual remaining queue. It never turns a speculative
native future into retail evidence. Seed 27718's retained spell-58 hit supplies
exact queues at 23,190 and 23,904. Native paired phase models favor short futures
in two of three windows, but retain declared initial planner-state differences
and host-timing dependence. Repeat the same live profile on natural seeds before
claiming broader stability; retain every attempt.

## P0: Attribute frame pacing and establish a 60-FPS gate

Actual update/presentation clocks and a v4 gate already exist for
[pure playback](REPLAY_PACING.md). The audio-off Lunatic pass and audio-on
recording failures do not establish online FPS. Reuse those measurement
contracts when investigating live control; the initial online receipt summaries
alone do not identify the cause of a stall.

Compare an identical route/configuration with playback, follower, search and
capture stages explicitly labeled. Correlate gaps with search launches, forks,
page faults, scheduling, rendering and capture. Retain full-run clocks plus
separate startup/dialogue/handoff/combat windows. Do not hide loading gaps or
use new random seeds as paired pacing controls. Add only the measurements needed
to distinguish a hypothesis; preserve source clocks and gameplay semantics.
Live `--render-timing` now supplies the shared update/draw/Present journals.
The short seed-61531 prefix has draw mean 0.76 ms and Present-call mean 19.18 ms,
with a failing gate. Add the independent window clock before any pacing claim;
this functional smoke is not a paired renderer or complete-game comparison.

The live runtime selector now supports the pinned Wine 11 wow64 / DXVK setup
and an independent X11 damage clock. Native-checked opening prefixes measure
51.96 Hz with default Wine 8 and 55.95 Hz with DXVK; both gates fail. Two pure
replay controls of the same seed-14702 input prefix, without MPC/follower,
measure 58.97 Hz (hardware bullets) and 59.62 Hz (CPU batching), still window
p99 28 ms and failing gates. Cache warmth and host load remain confounders.
Keep batching replay-only until a live change has stronger evidence, and inspect
render/host scheduling alongside actual decision and queue costs.

Acceptance: measured sustained combat presentation under a declared profile and
jitter tolerance, complete NMNB route correctness, intact lead/deadline rules
and verified resource bounds. Smooth media or fast decisions alone do not pass.

## P1: Improve earlier candidate choices within the deadline

Keep full native verification and independent literal digest replay. The tested
optional portfolio includes laser forecasts/orbits, power pickups, actual boss
HP/phase scoring, finite acceleration and survival-only longer lookahead.
Several flags retain failed seeds; global long horizons under the old cap also
fail. None is a general rescue merely because one combined profile clears.

Compare selected identical prefixes and bounded episodes before an actual Wine
sample. `--proposals` is a minimum evaluated-candidate target without anytime;
with `--anytime` the ranking reserve makes it best effort once a full future
exists. A certificate salvaged at timeout must have completed within the hard
budget, survived group cleanup and still pass retail lead/freshness checks.

Conditional `--recorded` geometry remains experimental. Keep source collision
phase timing, action/RNG dependence, 240-update / 2,048-box / 512-laser bounds,
five tape slots and the 32-MiB index cap. A cheap model cannot certify collisions.

Acceptance: include all retries, complete native winners, actual lead/adoption
behavior, timing distributions and total CPU. One new random retail seed is not
an A/B policy win. Do not broaden the full historical matrix as routine testing.

## Later coverage

Lunatic live evidence covers selected Stage 5/6 shots and one incomplete
Stage 1 → 2 story handoff. Extra has three full Reimu B clear samples. Neither
establishes all main-game stages/shots, arbitrary seeds or a complete live story.

The live runner captures the original retail seed and has no public `--seed`
selector. A maintained controlled common-start fixture remains open work;
never transplant pointer-rich managers or reseed a story handoff. Future stories
must carry original resources/rank/RNG through every natural transition and
reach the required terminal. Broader campaigns require a new declared scope.

## Code and evidence map

[Architecture](ARCHITECTURE.md#code-ownership) owns source entry points.
[Shadow MPC](SHADOW_MPC.md#boundary-contract) owns wire/lead/deadline behavior.
[Status](STATUS.md#next-work-and-evidence) indexes private evidence roots;
[Online history](ONLINE_HISTORY.md#historical-resume-commands-and-evidence)
retains the older named artifacts and all thirteen initial Lunatic trials.
The successful Extra case is `online-acceleration-reimuB-008`, under private
`reports/local/extra-online-20261027/`, with `proof.json`, actual inputs,
retail receipts, all decisions and frozen resource/binary identities.
