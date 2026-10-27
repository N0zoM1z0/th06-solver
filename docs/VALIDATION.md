# Build, validation and reproduction

[Status](STATUS.md) owns current acceptance. Commands here distinguish native
execution, original retail replay and live control. Original EXE/DATs, generated
tapes, logs and recordings stay under ignored `game_exe_data/`, `reports/local/`
or `build/`; only the two [selected replays](../replays/README.md) are tracked.

## Build and focused tests

Requires CMake 3.20+, Clang with Microsoft/x87 assembly extensions, an i386 C++
standard library and Ninja. On x86-64 Debian/Ubuntu, install `clang`,
`g++-multilib`, `cmake` and `ninja-build`. Native execution is headless and needs
no Wine, window or audio device.

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

To include the private real-DAT contracts:

```sh
cmake -S . -B build -DTH06_DATA_DIR=/path/to/private-th06-data
cmake --build build -j 2
ctest --test-dir build --output-on-failure -j 2
```

`TH06_DATA_DIR` contains the six original DAT archives, not already-extracted
members. CI runs the asset-free subset. The verified 2026-10-27 local build has
40 tests with DATs enabled; `ctest --test-dir build -N` lists the configured
suite. Keep affected real-DAT checks after behavior changes; a fresh clone
cannot access private historical tapes or probe harnesses.

The optional Wine bridges also require `i686-w64-mingw32-g++`, Wine and Xvfb:

```sh
cmake -S . -B build -DTH06_WINE_REPLAY=ON
cmake --build build -j 2
```

[`SHADOW_MPC.md`](SHADOW_MPC.md#run) documents the existing 32-bit live Wine
profile; [`REPLAY_PACING.md`](REPLAY_PACING.md#run-the-measured-replay) documents
the separate private Wine 11 WoW64 replay profile and required graphics libraries.
Use an owned game copy and dedicated prefix. Never attach to an unrelated game.

## Native entry profiles and complete proof

| Setting | Meaning |
| --- | --- |
| Difficulty IDs | Easy 0, Normal 1, Hard 2, Lunatic 3, Extra 4 |
| Shot IDs | Reimu A/B 0/1, Marisa A/B 2/3 |
| Standalone stages 1–6 | Explicit practice/checkpoint entry with the supplied power and seed |
| Stage 7 / difficulty 4 | Original Extra lifecycle with practice disabled; power 0 is the natural retail entry |
| `--story --stage 1` | Continuous main-game run; carry source RNG, rank and resources across handoffs |

The historical seed-0 matrix uses power 0/64/128 at stages 1/2/3–7. Its Extra
power-128 checkpoints are not the later normal-start online Extra proof.

```sh
build/th06_headless --data /path/to/private-th06-data --audit
mkdir -p reports/local
build/th06_plan --data /path/to/private-th06-data --stage 1 --difficulty 3 --shot 0 \
  --seed 0 --power 0 --frames 30000 --output reports/local/stage1.actions
build/th06_replay_check --data /path/to/private-th06-data \
  --tape reports/local/stage1.actions --require-complete
```

A native clear requires the original terminal event, zero misses/bombs and
fresh-process replay of every input/digest. A prefix, exhausted budget or hit
remains incomplete. `.actions` stores native inputs, identity and observable
digests; it is not a retail `.rpy` and cannot be renamed into one.

## Export and original-loader playback

The exporter accepts complete standard stories and complete normal-start Extra
(stage 7 / difficulty 4 / power 0). It reinitializes the native game, verifies
DAT identity and all tape digests, then writes the original T6RP format. Other
standalone checkpoints and incomplete tapes are rejected.

```sh
build/th06_replay_export --data /path/to/private-th06-data \
  --tape /path/to/complete-native-or-actual-online.actions \
  --output /path/to/private-game/replay/th6_export.rpy \
  --trace reports/local/native-export.csv
```

Export alone does not establish retail correctness. Run the original Wine replay
loader and compare its entire trace against the fresh export, including its
natural terminal. [Retail replay](RETAIL_REPLAY.md) explains the format and
[replay pacing](REPLAY_PACING.md) gives the measured command and recording flags.
Use `--start-stage 7` for Extra. Compact capture uses `--compact`; the explicit
owner-authorized `--memory-gib 4` option retains zero swap and a full resource
receipt. The default remains 2 GiB. Run one Wine instance at a time.

For actual online control, follow the [live protocol](SHADOW_MPC.md): new
retail starts capture their original seeds. The live runner has no public
`--seed` selector; private fixed-prefix native diagnostics are a separate kind
of evidence. Successful playback or media decode does not certify online FPS.

## Historical campaign reproduction

These commands describe earlier campaigns, not the next routine optimization.
Preserve the [seed-0 acceptance](NATIVE_HISTORY.md) and the separate
[Lunatic sampled-seed campaign](LUNATIC_RANDOM_SEEDS.md), including all failures
and retries. Do not schedule other difficulties or rerun a full matrix without
an explicit new request. Use at most four solver jobs on the shared host.

```sh
build/th06_campaign --data /path/to/private-th06-data --suite stages --difficulty 3 \
  --jobs 2 --output reports/local/stages --resume
build/th06_campaign --data /path/to/private-th06-data --suite stages --difficulty 3 \
  --jobs 2 --output reports/local/stages --resume --rewind 768 \
  --horizon 180 --proposals 8 --frames 120000
build/th06_campaign --data /path/to/private-th06-data --suite story --difficulty 3 \
  --jobs 2 --frames 180000 --reactive --horizon 120 --proposals 6 \
  --output reports/local/story --resume
```

Campaigns freeze their binaries and resume only after rechecking completed tapes.
A final continuation's timing is not the cost of its entire route. Native timing
models, original-loader playback and actual online runs retain separate ledgers.
