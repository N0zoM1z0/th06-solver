# th06-solver

C++17 native, headless Touhou 06 runtime, offline planning and live shadow MPC, following
[th08-solver](https://github.com/N0zoM1z0/th08-solver)'s native rollout approach.
**Native coverage: 96/96 legitimate stage/difficulty/shot combinations and 16/16
continuous stories** at seed 0. Every case is NMNB with a complete fresh-process
replay. **Wine retail playback also passes one complete Lunatic Reimu A story**:
127,737 updates, six natural stage clears and zero misses/bombs.
See [Retail replay](docs/RETAIL_REPLAY.md) for the separate evidence boundary.
The verified [Lunatic Reimu A `.rpy`](replays/th6_01.rpy) is included by the
owner's request; original game assets remain private. The current experiment is
Extra online shadow MPC, starting with Reimu A and the original retail entry.
Its first eight completed full attempts all hit; **Extra online NMNB is not yet established**.
Optional [laser-aware search](docs/EXTRA_SEARCH.md) passes the former Flandre
spell-55 failure in Wine samples, which later hit on spells 58/62.
Optional power-aware proposals, cooperative ranking, survival-only lookahead
boss-damage scoring and finite-acceleration guidance retain full native certificates. Their
[focused evidence and counterexamples](docs/EXTRA_SEARCH.md) remain separate
from a complete online clear.
The separate 2,400-update prefix passes follow and fresh native replay. Failed
cases, decision budgets and conditional recovery results are retained in the
[live controller guide](docs/SHADOW_MPC.md).
The [live Wine shadow controller](docs/SHADOW_MPC.md) clears two Lunatic Stage 5 /
Reimu B runs and one Stage 6 / Marisa B run NMNB, including an actual Stage 5
recording. The Stage 6 clear uses a minimum four-update decision interval, a 64-update
tail, eight-update lead and 80-ms search cap; search median/p99 is 29.07/77.70 ms.
Failed Stage 6 seeds remain counterexamples. These component runs are separate
from full-story retail replay and do not establish all-seed robustness.
**Online shadow MPC does not yet sustain stable 60 FPS.** Search deadlines,
successful clears and smooth capture timestamps do not establish game frame pacing.
The separate [Wine `.rpy` playback study](docs/REPLAY_PACING.md) isolates:
native verifies/exports first, then the actual retail replay runs without MPC or
a native follower. Playback FPS and online-controller FPS have separate acceptance.
One complete **Lunatic Reimu A story now passes the replay presentation gate**:
127,737 native-matching updates, six natural clears and zero misses/bombs.
Actual window output is 59.990 Hz, slowest five seconds 58.4 Hz and p99 18 ms.
The retained Stage 4 failure improves from 56.4 to 59.4 Hz over five seconds.
The measured profile uses private Wine 11 new WoW64, 64-bit lavapipe, original
CPU bullet drawing, bounded batches, scoped coarse sleeps, mapped journals and an
opt-in [DXVK EndScene submission patch](docs/DXVK_REPLAY_BUILD.md).
Peak Wine-tree memory is 468.1 MiB with zero OOM events; group CPU time falls
26.7% in the retained complete-story comparison. All handoff gaps remain in the
statistics, including a 145-ms maximum window interval. This is one measured
replay under the declared local tolerance; online MPC remains unverified.
See the [pinned runtime](docs/REPLAY_RUNTIME.json).
A [complete Wine recording](docs/REPLAY_PACING.md#record-the-replay) is now
verified with original game audio: 35:35.744, 640×480, 504-MB compact MP4.
Its audio-on handoff pacing remains below the gate; the replay file is unchanged.
The latest [search-volume study](docs/PERFORMANCE.md) retains verified native
future inputs with opt-in `--reuse 32`: 20 new paired cases clear 20/20 after
recovery, with 72% fewer speculative updates and 65% less planner CPU. First-pass
clears fall from 18/20 to 16/20, so retention remains optional. The earlier native
kernel optimizations make unchanged-input stage replays about 1.5–2× faster;
diverse-first ordering remains a separate optional experiment.
The preselected [random-seed experiment](docs/LUNATIC_RANDOM_SEEDS.md) covers
four seeds across all Lunatic stages/shots and continuous stories. Its standalone
matrix passes **96/96** (92 first-pass, four recovered), and its continuous stories
pass **16/16** (11 first-pass, five recovered), all with full fresh native replay.

## Continue development

The current owner-requested experiment is [Extra live shadow MPC](docs/SHADOW_MPC.md).
Keep its natural power-0 retail start, actual input receipts and decision deadlines
separate from historical power-128 native checkpoints and
[retail replay pacing](docs/REPLAY_PACING.md).

Start with the [engineering handoff and open work](docs/CONTINUATION.md): it maps
completed work, the historical Lunatic live ledger, retained failures, source entry points, private
evidence and acceptance criteria. Priorities are stable 60-FPS online pacing,
repeatable failure/queue diagnostics, hard-seed robustness, then broader Lunatic
stage/shot coverage and natural live stories. The
[controller guide](docs/SHADOW_MPC.md) contains current run profiles.
Track follow-up work in
[issue #1: 60-FPS pacing, hard seeds and full-story control](https://github.com/N0zoM1z0/th06-solver/issues/1).

## Historical offline campaign

The commands below reproduce the earlier offline campaign; routine online work
should use selected hard cases rather than rerun the full matrix or other
difficulties. The resumable campaign runs 96 standalone cases first, then 16
main-game stories. Existing tapes are rechecked by the current native binary
when resuming.

```sh
build/th06_campaign --data /path/to/th06-data --suite stages --difficulty 3 --jobs 2 \
  --output reports/local/stages --resume
# Retry incomplete routes from a checked earlier prefix with a larger lookahead.
build/th06_campaign --data /path/to/th06-data --suite stages --difficulty 3 --jobs 2 \
  --output reports/local/stages --resume --rewind 768 --horizon 180 --proposals 8 --frames 120000
build/th06_campaign --data /path/to/th06-data --suite story --difficulty 3 --jobs 2 --frames 180000 \
  --reactive --horizon 120 --proposals 6 \
  --output reports/local/story --resume
```

The runtime imports [N0zoM1z0/th06](https://github.com/N0zoM1z0/th06) at
`9e79eb8d784b6346233f2e98e1bcda1d2f04da15`, targeting Japanese 1.02h. It executes
the reconstructed ECL, ANM, player, enemy, bullet, laser, item and GUI update code.
Linux i386 preserves the original layouts and inline x87 helpers. The headless
runtime needs no window, graphics/audio device, Wine or Python runtime. The live
controller and capture use the separate Wine/Python tooling described above.

## Build

On x86-64 Debian/Ubuntu, install `clang`, `g++-multilib`, `cmake` and `ninja-build`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

Provide your own six original DAT archives in a separate directory. They and the
EXE are never distributed here. The local archive supplied for development is
kept under ignored `game_exe_data/`.

```sh
build/th06_headless --data /path/to/th06-data --audit
mkdir -p reports/local
build/th06_plan --data /path/to/th06-data --stage 1 --difficulty 3 --shot 0 \
  --seed 0 --power 0 --frames 30000 --output reports/local/stage1.actions
build/th06_replay_check --data /path/to/th06-data \
  --tape reports/local/stage1.actions --require-complete
```

Difficulty IDs are Easy 0, Normal 1, Hard 2,
Lunatic 3 and Extra 4. Shot IDs are Reimu A/B 0/1 and Marisa A/B 2/3. Stage 7 uses
Extra difficulty. Standalone starts are explicit checkpoints; `--story` starts a
continuous main-game run at stage 1 with carried resources and RNG.

## Verification and current scope

To enable the optional real-DAT tests:

```sh
cmake -S . -B build -DTH06_DATA_DIR=/path/to/th06-data
cmake --build build -j 2
ctest --test-dir build --output-on-failure -j 1
```

The complete standalone matrix includes all four Extra shots, with complete
fresh-checked routes of 47,240–61,696 updates. Story acceptance additionally
requires the natural five-stage Easy or six-stage Normal/Hard/Lunatic terminal.
All 16 difficulty/shot stories pass, starting at power 0 and carrying actual
resources, rank and gameplay RNG through every stage.
Coverage, exact checkpoint profiles and reproduction
belong in [Status](docs/STATUS.md). [Architecture](docs/ARCHITECTURE.md) describes
the native profile, checkpoints, proposals and lifecycle boundaries;
[third-party notices](docs/THIRD_PARTY_NOTICES.md) record source provenance.

Tapes are versioned native action files, not retail `.rpy` files. Matching native
execution validates reproducibility under this source profile, not retail accuracy.
`th06_replay_export` verifies a complete native story and writes a source-format
retail `.rpy`. Export alone does not certify retail playback.
