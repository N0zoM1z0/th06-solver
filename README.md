# th06-solver

C++17 native Touhou 06 runtime, offline planning and live Wine shadow MPC,
following [th08-solver](https://github.com/N0zoM1z0/th08-solver)'s common-start,
true-RNG rollout approach. The original game decides collisions and outcomes;
isolated native futures propose verified inputs to the owned retail process.

**Extra has three complete online NMNB clears:** Reimu B, captured retail seeds
35964, 61515 and 57265, natural power-0 entry, 43,126 / 44,959 / 47,986 updates.
All three actual input routes pass complete fresh native replay. Seed 35964 also passes
original-loader Wine `.rpy` playback.

| Verified result | Scope |
| --- | --- |
| Extra online NMNB | Three Reimu B seeds clear; thirteen failed full attempts retained |
| Lunatic online NMNB | Two Stage 5 / Reimu B clears and one Stage 6 / Marisa B clear; no complete live story |
| Retail replay | Complete Lunatic Reimu A story and Extra Reimu B route |
| Replay presentation gate | One audio-off Lunatic story passes; online 60 FPS and audio-on recording pacing remain unresolved |
| Historical native coverage | Seed-0 checkpoints 96/96 and continuous stories 16/16; separate four-seed Lunatic campaign also passes after recovery |

Selected input-only replays: [Lunatic Reimu A](replays/th6_01.rpy) ·
[Extra Reimu B](replays/th6_02.rpy).
[Replay provenance](replays/README.md) and
[compact recording details](docs/REPLAY_PACING.md#record-the-replay) include the
35:35.744 / 504-MB story and 12:04.267 / 172-MB Extra videos, both with game audio.
Original assets, recordings and raw experiment data stay private.

A sampled clear does not establish random-seed robustness or all-stage/shot
retail coverage. [Status](docs/STATUS.md) owns current results, limitations and
next priorities; offline, replay and online evidence remain distinct.

## Start here

Requires Linux x86-64, CMake 3.20+, Clang, an i386 C++ standard library
(`g++-multilib` on Debian/Ubuntu), and Ninja. Native execution also needs your own
six original DAT archives. Wine and MinGW are optional for the retail bridges.

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

[Validation](docs/VALIDATION.md) covers real-DAT tests, focused native planning,
complete replay checks, `.rpy` export and historical campaign reproduction.
[Live shadow MPC](docs/SHADOW_MPC.md#run) covers the actual Wine controller;
[Extra search](docs/EXTRA_SEARCH.md#first-complete-extra-online-nmnb-clear)
contains the successful combined profile and its failed controls.

## Documentation

| Document | Responsibility |
| --- | --- |
| [Status](docs/STATUS.md) · [Continuation](docs/CONTINUATION.md) | Current evidence, open work and operational restart checklist |
| [Validation](docs/VALIDATION.md) | Build, entry profiles, native proof and export commands |
| [Architecture](docs/ARCHITECTURE.md) | Source/runtime ownership, proposals, lifecycle and proof boundaries |
| [Shadow MPC](docs/SHADOW_MPC.md) · [Extra search](docs/EXTRA_SEARCH.md) | Online protocol, settings, measured outcomes and retained failures |
| [Retail replay](docs/RETAIL_REPLAY.md) · [Replay pacing](docs/REPLAY_PACING.md) | Original-loader interoperability, presentation and compact recording |
| [Performance](docs/PERFORMANCE.md) · [Lunatic seeds](docs/LUNATIC_RANDOM_SEEDS.md) | Focused native timing studies and sampled seed campaign |
| [Native history](docs/NATIVE_HISTORY.md) · [Online history](docs/ONLINE_HISTORY.md) | Earlier acceptance tables, experiment costs and handoff notes |
| [Engineering rules](AGENTS.md) · [Third-party notices](docs/THIRD_PARTY_NOTICES.md) | Contribution rules, licenses and pinned provenance |

The runtime imports [N0zoM1z0/th06](https://github.com/N0zoM1z0/th06) at
`9e79eb8d784b6346233f2e98e1bcda1d2f04da15`, targeting Japanese 1.02h. The
32-bit native build preserves original layouts, callbacks and gameplay RNG.
Only the two owner-selected generated `.rpy` files are tracked.

[Open tracking issue](https://github.com/N0zoM1z0/th06-solver/issues/1)
