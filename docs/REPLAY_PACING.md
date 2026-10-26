# Wine retail replay frame pacing

The owner's immediate target is **native solve/export → actual Lunatic `.rpy`
playback → stable 60-FPS window output**. No online MPC, native follower or fixed
input fixture runs during this acceptance. Online-controller deadlines, queue
behavior and FPS remain separate unfinished work in
[issue #1](https://github.com/N0zoM1z0/th06-solver/issues/1).

A complete retail clear and a passing timing gate are independent requirements.
Several complete Stage 6 runs match the native trace and still fail frame pacing.
A good 3,000-update timing prefix is not a complete-stage result.

## Run a selected replay

Use the owned Japanese 1.02h game and original DATs described in
[RETAIL_REPLAY.md](RETAIL_REPLAY.md). Game assets, runtime binaries and preparation
artifacts stay private. Verify/export the existing native tape before Wine:

```sh
cmake -S . -B build -DTH06_WINE_REPLAY=ON
cmake --build build -j 2
build/th06_replay_export --data /path/to/private-th06-data \
  --tape reports/local/story/story-d3-shot0-seed0.actions \
  --output /path/to/private-game/replay/th6_paced.rpy \
  --trace reports/local/native-export.csv
```

The verified story has 127,737 updates and reproduces the published replay hash
`c29f3975d9b2744d3460c7946c7130bdc026dc3c47566c1e0d45bf7c14fb192d`.
Playback does not launch that native binary again.

The current runtime candidate uses private Wine 11.0 in new WoW64 mode, native
DXVK 2.4 D3D8/D3D9 DLLs and 64-bit Mesa 25.0.7 lavapipe. Initialize a separate
64-bit prefix under the same memory cap. Package/DLL hashes, upstream licenses
and settings are pinned in [REPLAY_RUNTIME.json](REPLAY_RUNTIME.json). Wine's
[release notes](https://raw.githubusercontent.com/wine-mirror/wine/wine-11.0/ANNOUNCE.md)
document `WINEARCH=wow64`; it cannot use the earlier pure 32-bit prefix. Keep the
original game/config/replay unchanged in a private copy containing the two
external DLLs and `dxvk.conf`:

```ini
dxvk.numCompilerThreads = 1
d3d9.maxFrameLatency = 1
d3d9.maxFrameRate = -1
d3d9.presentInterval = 0
```

Point a private ICD JSON at the **64-bit** `libvulkan_lvp.so`. The Windows game,
observer and D3D8/D3D9 DLLs remain 32-bit; the Unix graphics libraries are 64-bit.
The runtime is selected explicitly, including its matching wineserver:

```sh
LD_LIBRARY_PATH=/path/to/private-mesa/usr/lib/x86_64-linux-gnu \
VK_ICD_FILENAMES=/path/to/private-lavapipe-amd64.json \
WINE_D3D_CONFIG=csmt=0 python3 scripts/run_wine_replay.py \
  --wine /path/to/private-wine/bin/wine --wine-arch wow64 \
  --game /path/to/private-game --prefix /path/to/private-prefix64 \
  --dll-overrides 'd3d8,d3d9=n;mscoree,mshtml=' \
  --replay replay/th6_paced.rpy --start-stage 6 --single-stage \
  --native-trace reports/local/native-export.csv \
  --renderer-threads 4 --vblank off --precise-limiter --display-timing \
  --shader-cache reports/local/replay-cache --require-pacing \
  --updates 30000 --seconds 900 --output reports/local/replay-stage6
```

`--start-stage` uses the original replay loader's stage records, including carried
resources, rank, RNG and previous-stage score. It does not introduce a practice
checkpoint or movement inputs. `--single-stage` stops at the natural clear and
records that scope. Omit both stage-selection options for a full story; use
`--updates 180000 --seconds 3000`. Budgets are limits, not completion criteria.
Use a fresh output directory for every attempt. `--snapshot` adds one screenshot
inside the initial 120-update warmup and the same capped group.

The legacy Wine 8 / 32-bit mode remains available without `--wine` or
`--wine-arch`. Native DLL selection is explicit; the default remains the older
builtin renderer. Actual loaded paths/hashes, not the requested environment alone,
establish which renderer ran.

## Clocks and the limiter correction

The original loop draws, runs its calculation chain, waits for its 60-Hz deadline
and presents. Calculation callback intervals, render submission time and output
intervals therefore measure different phases. The D3D8 profiler, adapted from the
pinned TH08 implementation, forwards every BeginScene, EndScene and Present call
and result. Its draw span includes submission/synchronization; neither that span
nor Present duration is a GPU-only timer. An SDK ABI check verifies the i386 slots.

An asynchronous command stream can return from Present after enqueueing. Wine 8's
[command-stream implementation](https://github.com/wine-mirror/wine/blob/wine-8.0/dlls/wined3d/cs.c#L680)
separates that operation from backend completion. `--display-timing` independently
observes XDamage notifications on the actual 640×480 game window. It records X
server timestamps and receiver timestamps for real drawable updates, without a
sampling rate, generated frames or input injection. Output counts are compared
with the API journal. Recorder timestamps and the game's FPS display are separate
diagnostic evidence.

The pinned limiter also loses precision at long uptimes. D3D8 selects 24-bit x87
arithmetic, while the source accumulates an absolute millisecond timestamp in a
`double`. At the observed uptime above 442 million ms, adding 16.6667 ms can round
to 32 ms. An actual x87 test reproduces this defect.

`--precise-limiter` widens only that original limiter arithmetic to 53 bits. The
hook matches the pinned `timeBeginPeriod` return address, forwards the original
API call/result, and restores the exact prior control word before BeginScene,
Present and subsequent gameplay. It leaves source time APIs, the 60-Hz constant,
clock values, frame skip, input, collisions and RNG intact. The option requires
zero frame skip. Entry/restoration counters and a gameplay scope guard check the
boundary. Integer quotient/remainder conversion keeps profiling timestamps
independent of the game's x87 mode.

## Acceptance and resource scope

Version 4 retains the original numerical presentation thresholds:

- At least 30 seconds after the first 120 calculation callbacks.
- Mean output between 59.5 and 60.5 Hz; slowest complete five-second window at
  least 58 Hz, shifting windows one second at a time.
- Output interval p99 at most 20 ms.
- No missing draw, failed Present or repeated render context flags.

An independent window clock must also satisfy those cadence thresholds, show full
window updates, and differ from steady API output counts by at most three. A
requested window clock must pass. Without one, API timing is accepted only with
an explicitly confirmed synchronous command stream. `pacing_gate`, `display_gate`
and `acceptance_gate` remain distinct in the report. `--require-pacing` fails the
command when the relevant acceptance gate fails, even after a natural clear.

Full statistics retain every interval. Only the first 120 callbacks are omitted
from the separately labeled steady result. Maximum gaps and counts above
20/33.3/50 ms remain visible. This is a declared local timing tolerance, not an
every-frame 16.67-ms guarantee. Calculation jitter remains separate because
replay/dialogue can restart the original calculation chain.

Gameplay comparison covers all 15 exported fields: finite positions within 0.025
game units, other fields exact. Selected-stage global frame numbers are normalized
to that slice. Partial matching prefixes stay incomplete. A complete result also
requires the original natural clear, zero misses/bombs and the full expected
native route. Native internal digest agreement and retail equivalence are separate.

One owned Wine tree runs at a time. Both launchers share a project lock, with the
legacy prefix lock retained. Wine, Xvfb, the frozen worker, display observer and
optional screenshot share **2 GiB hard / 1.5 GiB high / no swap**, finite runtime
and a 256-task cap. No solver/controller is launched. The runner verifies actual
cgroup limits and records memory/OOM, CPU usage, host load/pressure and cache state.

Systemd services inherit their manager's environment. The runner explicitly
normalizes/forwards renderer settings and checks the effective worker environment.
Architecture-specific library paths apply only to the Wine child; the 64-bit
Xvfb/Python tools keep their own library environment. Wine version, prefix mode,
loaded Unix ELF class, actual graphics-library hashes, EXE/RPY/DAT/config/DLL
identities and frozen tooling accompany each run.

## Evidence and retained failures

The private ledger is `reports/local/pacing-20261026/rpy-attempt-ledger.json`;
each attempt retains raw clocks, trace, logs, hashes and resource receipts.
The Wine 11 new-WoW64 / DXVK / 64-bit lavapipe profile passes a complete
Stage 6. Full-story pacing is the next acceptance check; it has not yet passed.

| Attempt | Native-matching retail route | Mean Hz | Slowest 5 s Hz | p99 ms | Result |
|---|---:|---:|---:|---:|---|
| Wine 8 Stage 5 baseline prefix | 3,000 prefix | 54.85 | 45.2 | 31.54 | Fail |
| Wine 8 Stage 5 precise limiter, synchronous API | 20,045 complete | 59.86 | 58.0 | 19.09 | Local API gate pass |
| Wine 8 Stage 6 precise limiter, synchronous API | 24,441 complete | 59.74 | 56.2 | 20.07 | Fail |
| Wine 8 + Mesa 25 OpenGL Stage 6 | 24,441 complete | 58.17 | 43.2 | 26.65 | Fail |
| Wine 8 + DXVK / 32-bit lavapipe Stage 6 window | 24,441 complete | 59.47 | 51.6 | 22 | Fail |
| Wine 11 WoW64 OpenGL Stage 6 window | 24,441 complete | 56.69 | 39.8 | 30 | Fail |
| Wine 11 WoW64 + DXVK / 64-bit lavapipe Stage 6 window | 24,441 complete | 59.96 | 58.6 | 19 | Pass |

The passing Stage 6 run peaks at 350,384,128 bytes with zero OOM events; its
24,536 precision entries all restore before gameplay. Steady output still contains
a 119-ms maximum gap and five gaps above 50 ms, retained in the report. The gate
does not certify that every frame is exactly 16.67 ms.

Earlier vertex-color, CPU-transform and quad-batch experiments, limiter-sleep
variants and one/eight-thread controls also failed; their code is archived
privately and is absent from the current implementation. A two-thread GDB trial
was already failing before read-only profiling; it is diagnostic, not a timing
control. A timer API microbenchmark ruled out repeated resolution calls as a
meaningful cost (149 versus 112 ns per poll). Its cleanup timeout is retained.

Three early renderer controls missed the systemd environment boundary and are
explicitly marked invalid. The builtin Vulkan control failed before its first
frame. DXVK can initialize Vulkan separately; that earlier assertion does not
establish that all Vulkan paths are unavailable. Earlier `reference-*` follower
fixtures predate pure-RPY acceptance and never count as playback successes.

## Artifacts and checks

| Artifact | Meaning |
|---|---|
| `retail.csv`, `native-comparison.json` | Actual replay projection; complete-route/prefix distinction |
| `update-times.bin` | Versioned header and three uint64 words per callback |
| `render-times.bin` | Versioned header and eight uint64 words per API presentation |
| `display-times.csv` | Raw XDamage server/receiver times, latest callback and geometry |
| `pacing.json` | Full/steady distributions, independent clocks and versioned gates |
| `clock.json`, `outcome.json` | Gameplay FPU mode, graphics options, natural result and precision counters |
| `effective-*-environment.json` | Worker and explicitly constructed Wine-child environments |
| `wine-runtime.json`, `graphics-libraries.json` | Runtime version, Unix bitness and actual loaded library hashes |
| `resource-receipt.json` | Scope, limits, identities, cache state, peak memory/OOM and host conditions |

```sh
python3 scripts/replay_pacing.py reports/local/replay-stage6
ctest --test-dir build --output-on-failure \
  -E 'real_data_contracts|story_transition_contracts|story_terminal_contracts|extra_bootstrap_contracts'
```

Twenty-five focused contracts pass, including the real x87 regression/restoration,
integer timing, synthetic 60/30-Hz discrimination, rejection of asynchronous API
receipts without output proof, malformed streams, selected native-route comparison
and missing service environment rejection. Actual retail cases remain necessary.
