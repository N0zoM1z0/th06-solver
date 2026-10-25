# th06-solver

C++17 native, headless Touhou 06 runtime and offline planning, following
[th08-solver](https://github.com/N0zoM1z0/th08-solver)'s native rollout approach.
The immediate goal is every legitimate stage/difficulty/shot combination, then
complete continuous stories. Retail integration is deferred.

The resumable campaign runs the 96 standalone cases first, then 16 main-game
stories. Existing tapes are rechecked by the current native binary when resuming.

```sh
build/th06_campaign --data /path/to/th06-data --suite stages --jobs 2 \
  --output reports/local/stages --resume
build/th06_campaign --data /path/to/th06-data --suite story --jobs 2 --frames 180000 \
  --output reports/local/story --resume
```

The runtime imports [N0zoM1z0/th06](https://github.com/N0zoM1z0/th06) at
`9e79eb8d784b6346233f2e98e1bcda1d2f04da15`, targeting Japanese 1.02h. It executes
the reconstructed ECL, ANM, player, enemy, bullet, laser, item and GUI update code.
Linux i386 preserves the original layouts and inline x87 helpers. No window,
graphics/audio device, Wine or Python runtime is needed.

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

The first Lunatic stage-1 seed-0 Reimu-A route finishes 13,701 native updates with
zero misses and bombs, and passes a fresh-process per-frame replay. This is one
case, not a completed coverage matrix. Full current coverage and reproduction
belong in [Status](docs/STATUS.md). [Architecture](docs/ARCHITECTURE.md) describes
the native profile, checkpoints, proposals and lifecycle boundaries;
[third-party notices](docs/THIRD_PARTY_NOTICES.md) record source provenance.

Tapes are versioned native action files, not retail `.rpy` files. Matching native
execution validates reproducibility under this source profile, not retail accuracy.
