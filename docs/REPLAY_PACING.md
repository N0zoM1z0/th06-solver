# Wine retail replay frame pacing

The owner's immediate target is **native solve/export → actual Lunatic `.rpy`
playback → stable 60-FPS window output**. No online MPC, native follower or fixed
input fixture runs during this acceptance. Online-controller deadlines, queue
behavior and FPS remain separate unfinished work in
[issue #1](https://github.com/N0zoM1z0/th06-solver/issues/1).

The normal complete **Lunatic Reimu A seed-0 story passes both unchanged v4
presentation gates**: 127,737 native-matching updates, six natural clears, NMNB.
API output is 59.990 Hz / slowest 5 s 58.6 Hz / p99 17.130 ms; actual window
output is 59.990 Hz / slowest 5 s 58.4 Hz / p99 18 ms. All window updates cover
640×480, render error flags are zero and steady output counts differ by one.
Peak is 490,835,968 bytes (468.1 MiB), OOM/OOM-kill 0 under verified 2-GiB/no-swap
limits. Group CPU is 3,993.08 s, 26.7% lower than the retained earlier complete
profile. This run uses no accelerated prefix, MPC or native follower.

Every later gap remains in the gate, including a 145-ms maximum window interval
at a stage handoff. The result establishes the declared local timing tolerance
for one literal replay, not an every-frame 16.67-ms guarantee or online MPC FPS.

A complete retail clear and a passing timing gate are independent requirements.
Several complete Stage 6 runs match the native trace and still fail frame pacing.
A good 3,000-update timing prefix is not a complete-stage result.

## Run the measured replay

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

The accepted runtime uses private Wine 11.0 in new WoW64 mode, an
[opt-in locally patched DXVK 2.4 D3D8 bridge](DXVK_REPLAY_BUILD.md), official
D3D9 and 64-bit Mesa 25.0.7 lavapipe. Initialize a separate
64-bit prefix under the same memory cap. Package/DLL hashes, upstream licenses
and settings are pinned in [REPLAY_RUNTIME.json](REPLAY_RUNTIME.json). Wine's
[release notes](https://raw.githubusercontent.com/wine-mirror/wine/wine-11.0/ANNOUNCE.md)
document `WINEARCH=wow64`; it cannot use the earlier pure 32-bit prefix. Keep the
original game/config/replay unchanged in a private copy containing the two
external DLLs and `dxvk.conf`:

```ini
dxvk.numCompilerThreads = 1
d3d9.maxFrameLatency = 2
d3d9.maxFrameRate = -1
d3d9.presentInterval = 1
d3d8.earlyFlush = True
```

The profile uses the original `dontUseVertexBuf` option: graphics opts `0x883`
instead of the earlier `0x881`, selecting per-vertex diffuse colors. Keep the
source game/config unchanged and use a private copy. Resolution, 32-bit color,
frame skip and image quality are retained. The bridge option is implemented by
the supplied source patch; stock DXVK 2.4 does not implement `d3d8.earlyFlush`.

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
  --replay replay/th6_paced.rpy \
  --native-trace reports/local/native-export.csv \
  --renderer-threads 8 --vblank off --sleep-limiter --display-timing --batch-bullets \
  --shader-cache reports/local/replay-cache --require-pacing \
  --updates 180000 --seconds 3000 --output reports/local/replay-story
```

For a selected-stage diagnostic, add `--start-stage 4 --single-stage` and use
`--updates 35000 --seconds 900`. `--start-stage` uses the original replay loader's stage records, including carried
resources, rank, RNG and previous-stage score. It does not introduce a practice
checkpoint or movement inputs. `--single-stage` stops at the natural clear and
records that scope. The measured full-story command omits both selection options.
Budgets are limits, not completion criteria.
Use a fresh output directory for every attempt. `--snapshot` adds one screenshot
inside the initial 120-update warmup and the same capped group.

The legacy Wine 8 / 32-bit mode remains available without `--wine` or
`--wine-arch`. Native DLL selection is explicit; the default remains the older
builtin renderer. Actual loaded paths/hashes, not the requested environment alone,
establish which renderer ran.

## Record the replay

Add `--record` to capture the owned 640×480 game rectangle and private stereo game
audio in separate processes. Add `--compact` instead for a full-size, original-
speed H.264 fast-CRF26/AAC128k delivery, following TH08's compact profile. It
implies `--record`; capture remains ultrafast/single-thread, then compact encoding
runs after Wine exits. Actual capture timestamps are retained without interpolation.
Use `--seconds 5400` for a full story so the same bounded group can finish capture
and subsequent encoding. The previous no-record pacing acceptance remains separate.

Prepare a separate owned game copy with original WAV music and effects enabled;
the pacing baseline has audio disabled. The recorder routes only that Wine child
to a dedicated PulseAudio null sink. No host microphone or other application audio
is recorded. The default 2-GiB/no-swap cgroup covers Wine, Xvfb, PulseAudio,
both capture processes and the final encoder. For the owner-authorized Extra
recording, `--memory-gib 4` selects a 4-GiB hard / 3-GiB high cap; the verified
resource receipt records the actual limit and peak. Swap remains disabled. The recorder uses the root rectangle so startup
window recreation cannot invalidate its capture target.

The output retains `video.mkv`, lossless `audio.wav`, clock-origin/alignment data
in `capture.json`, and `gameplay-compact.mp4` (or stream-copy `gameplay.mp4` without
`--compact`). Each run freezes the capture helper with its SHA-256. Recording
process failure fails the run; stopped/failed footage remains distinct from a
complete clear. Verify complete native agreement and terminal state separately
from full media decode, both streams, audible audio and capture PTS/cadence.

The complete 2026-10-27 recording is retained at
`reports/local/replay-video-20261027/story-compact001/`, with the delivery hard-linked
as `reports/local/replay-video-20261027/th06-lunatic-reimuA-story-compact.mp4`.
The published replay SHA-256 is unchanged. Wine again clears all six stages NMNB
and matches all 127,737 native updates. The 503,859,305-byte MP4 is 2,135.744 s;
all 128,141 frames decode, and every relative capture PTS matches the source.
Capture intervals have p99/max 17/36 ms; AAC stereo audio is audible at −10.6 dB
mean. Visual checks include the Sakuya fight and final `All Clear!` screen.

This audio-on recording is **not a passing pacing run**: actual window output is
59.928 Hz / slowest 5 s 56.4 Hz / p99 19 ms / maximum 305 ms, below the unchanged
58-Hz five-second minimum. Capture cadence does not remove those game handoff
gaps. The game and encoder exit successfully; `--require-pacing` returns 1.
The combined group peaks at 1,611,669,504 bytes with zero OOM/OOM-kill events under
verified 2-GiB/no-swap limits. Group CPU includes post-game encoding and must not
be compared with the no-record baseline. The original 1,589,766,991-byte video,
409,985,558-byte PCM audio, successful 2,400-update prefix smoke and full recording
receipts remain private; no new solving campaign ran.

## Extra online-route recording

The owner-requested [`th6_02.rpy`](../replays/th6_02.rpy), published in
`61c0eaefc46fd3df597e3748635967cf9117a7b1`, records actual Extra online inputs:
Reimu B, seed 35964, original power 0, complete NMNB at 43,126 updates.
The original retail replay loader under Wine 11 independently reaches the
natural replay-menu return with every declared native projection field matching.
This recording uses no MPC or native follower; export verification occurs first.

Use the Wine 11/Mesa/DXVK/audio profile above with `--start-stage 7`,
`--replay replay/th6_02.rpy`, its fresh `--native-trace`, `--compact`,
`--memory-gib 4`, `--updates 50000` and `--seconds 2400`. Extra stores one record
in the original seventh stage slot; it is not a main-game practice checkpoint.
The default memory limit remains 2 GiB. The owner's explicit 4-GiB recording
allowance applies to Wine, display, private audio, capture and post-game encoder,
with a 3-GiB high watermark and swap disabled.

The complete private case is `reports/local/extra-video-20261027/extra-compact001/`.
Its compact delivery is hard-linked as
`reports/local/extra-video-20261027/th06-extra-reimuB-replay-compact.mp4`:

| Check | Result |
|---|---|
| Delivery | 12:04.267, 172,025,731 bytes, 640×480 H.264 / AAC stereo |
| Original playback | 43,126 updates, one natural Extra clear, zero misses/bombs |
| Complete source/compact/decode frames | 43,454 / 43,454 / 43,454 |
| Relative capture timestamps | Identical; zero duplicates, max error 0 µs |
| Capture interval p99 / max | 18 / 57 ms |
| Game audio mean | −10.4 dB; original WAV music/effects, private audio sink |
| Whole recording/encoding peak | 1,173,569,536 bytes, 1,119.20 MiB |
| Verified hard cap / swap / OOM / OOM-kill | 4 GiB / 0 / 0 / 0 |
| Limiter scopes / drawing scopes | All 43,192 / 43,072 restore |

The MP4 SHA-256 is `9186dcdfbc6c655f52577111515be183acb7167b03ef2a203f98f9a6adc333eb`.
Visual checks include the Flandre fight and final `All Clear!` screen. The
1,200-update smoke passes; an earlier ten-update attempt stopped because the
monitor rejected valid stage 7. Both are retained in `recording-attempts.json`,
with a regression accepting stage 7 and rejecting stage 8. A corrected VFR
null-output decode check avoids quantizing valid capture timestamps to 60-Hz
output ticks; all final media checks pass without changing the video.

Recording presentation is separate from route and media acceptance: actual
window output is 59.972 Hz / slowest 5 s 57.4 Hz / p99 19 ms / maximum 223 ms,
so this audio-on recording does not pass the unchanged pacing gate. It also
does not establish online-controller FPS or another online clear. All raw media,
receipts, checks and the delivery manifest remain ignored; only the two explicitly
authorized input-only replay files are tracked.

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

`--sleep-limiter` implies that precision correction and adds `Sleep(1)` only
when the same millisecond clock is more than 4 ms before the original deadline.
The original final checks still busy-wait; overdue frames and clock wrap skip
the sleep. It leaves the deadline accumulator and original Present decision
unchanged. The complete Stage 4 comparison reduces limiter polls from
177,352,596 to 33,864,904 and whole-group CPU time from 1,195.52 to 871.67 seconds
(27.1%); 304,422 sleeps are recorded. Both render clocks pass, all native fields
match, and all 28,397 precision entries restore. This is one measured comparison,
not a host-independent CPU reduction; coarse sleep alone still fails the
earlier full-story counterexample. The final combined profile passes separately.

## Bounded mapped collection

The new collector copies fixed-size records into preallocated mapped journals and
publishes their committed count with a 32-bit interlocked store. It removes the
two per-frame Windows `WriteFile` calls and per-frame CSV number formatting.
The 15-field state record is 60 bytes; update and render records retain their
24/64-byte layouts. Version 2's 16-byte header is magic/version/stride/count;
readers ignore the uncommitted tail, and normal close flushes/truncates it.
Old version-1 journals remain readable with their original strict checks.

Capacity derives from the declared update budget, with 512 extra render slots;
exhaustion fails the run. The default 180,000-update budget reserves about
25.4 MiB across all three files. The display observer keeps one read-only mapping
and reads the last committed update, instead of reopening the timing file per
window event. Its CSV flushes once per 60 events and on exit. State CSV conversion
and native comparison run after Wine exits; every recorded field remains available.
Clocks, timestamps and version-4 acceptance thresholds are unchanged.

The mapped-collector milestone passed 26 focused contracts, including
publication before consumption, invalid counts, unused-tail rejection and typed CSV
conversion. A real Wine diagnostic prefix retains all 78,000 native-matching NMNB
updates, peaks at 279,580,672 bytes and reports zero OOM events. Fast diagnostic
prefixes skip early raster work and waits to reach the retained failure quickly; they
are explicitly excluded from FPS acceptance. The normal complete story above provides
the separate final output proof.

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
optional screenshot share **2 GiB hard / 1.5 GiB high / no swap** by default
(or explicitly authorized `--memory-gib 4`: 4 GiB / 3 GiB / no swap), finite runtime
and a 256-task cap. No solver/controller is launched. The runner verifies actual
cgroup limits and records memory/OOM, CPU usage, host load/pressure and cache state.

Systemd services inherit their manager's environment. The runner explicitly
normalizes/forwards renderer settings and checks the effective worker environment.
Architecture-specific library paths apply only to the Wine child; the 64-bit
Xvfb/Python tools keep their own library environment. Wine version, prefix mode,
loaded Unix ELF class, actual graphics-library hashes, EXE/RPY/DAT/config/DLL
identities and frozen tooling accompany each run.

## Original CPU drawing and bounded batching

The original hardware bullet draw changes its world matrix for each sprite.
`--software-bullets` selects the original `DrawBulletNoHwVertex` branch only in
the priority-9 BulletManager draw callback, restoring the exact one-byte hardware
flag afterward. A guard rejects any scope reaching gameplay. ECL, inputs, RNG,
collision predicates and callback order remain owned by the original EXE.
The pinned DXVK fixed-function implementation
[bypasses texture transforms for XYZRHW positions](https://github.com/doitsujin/dxvk/blob/v2.4/src/d3d9/d3d9_fixed_function.cpp#L1130),
so the source's CPU-generated UVs are not transformed twice.

`--batch-bullets` implies that drawing branch and copies at most 512 adjacent
compatible quads into a triangle list, preserving every vertex byte, triangle
order and winding. Both original 24-byte factor-color and 28-byte diffuse-color
formats are supported. Every other SDK method flushes first, including state
changes and queries; scope exit flushes before the original hardware flag can
reach subsequent gameplay. Deferred draw failures reject the run. No gameplay
objects or textures are copied or modified by the batch adapter.

Per-vertex colors matter: the complete Stage 4 factor-color control submits
6,587,465 quads in 369,121 batches and fails pacing. Diffuse colors reduce that
same geometry to 30,925 batches. Six workers plus diffuse batching pass the API
gate (59.998 Hz, p99 17.144 ms), but immediate-mode window output still fails at
p99 21 ms. FIFO with one queued frame also fails the complete stage despite a
passing prefix. FIFO with a two-frame cap passes the complete stage at window
p99 19 ms. Presentation mode is checked in the actual Vulkan log; acceptance
requires both clocks. This replay queue does not establish an online latency policy.

Optional `--wine-nice`, `--wine-cpus` and `--display-cpus` record effective game
priority/affinity and display-worker affinity. Non-overlapping physical cores
must be chosen for the current host. Nice 5 and a separate display core did not
pass the window gate in the retained controls; the final profile uses the
default priority and affinity.

## Evidence and retained failures

The private ledger is `reports/local/pacing-20261026/rpy-attempt-ledger.json`;
each attempt retains raw clocks, trace, logs, hashes and resource receipts.
The earlier Wine 11 new-WoW64 / DXVK / 64-bit lavapipe profile passes a complete
Stage 6 with four workers, original hardware drawing and immediate presentation.
Those earlier results remain historical. The mapped/coarse-sleep/eight-worker
profile with explicit EndScene submission now passes the entire natural story.

| Attempt | Native-matching retail route | Mean Hz | Slowest 5 s Hz | p99 ms | Result |
|---|---:|---:|---:|---:|---|
| Wine 8 Stage 5 baseline prefix | 3,000 prefix | 54.85 | 45.2 | 31.54 | Fail |
| Wine 8 Stage 5 precise limiter, synchronous API | 20,045 complete | 59.86 | 58.0 | 19.09 | Local API gate pass |
| Wine 8 Stage 6 precise limiter, synchronous API | 24,441 complete | 59.74 | 56.2 | 20.07 | Fail |
| Wine 8 + Mesa 25 OpenGL Stage 6 | 24,441 complete | 58.17 | 43.2 | 26.65 | Fail |
| Wine 8 + DXVK / 32-bit lavapipe Stage 6 window | 24,441 complete | 59.47 | 51.6 | 22 | Fail |
| Wine 11 WoW64 OpenGL Stage 6 window | 24,441 complete | 56.69 | 39.8 | 30 | Fail |
| Wine 11 WoW64 + DXVK / 64-bit lavapipe Stage 6 window | 24,441 complete | 59.96 | 58.6 | 19 | Pass |
| Same four-thread profile, story window | 91,553 prefix | 59.97 | 57.4 | 20 | Fail; stopped after irreversible pacing failure |
| Stage 4 CPU/diffuse batch, six workers, immediate | 28,296 complete | 59.99 | 59.8 | 21 | API passes; window fails |
| Same Stage 4 profile, nice 5 | 28,296 complete | 59.98 | 59.0 | 21 | Fail |
| Same Stage 4 profile, separate display core | 28,296 complete | 59.96 | 59.2 | 22 | Fail |
| Stage 4 CPU/diffuse batch, six workers, FIFO | 3,600 prefix | 59.99 | 59.8 | 18 | Timing passes; route incomplete |
| Same FIFO profile, one-frame cap | 28,296 complete | 59.93 | 54.2 | 21 | Fail |
| Stage 4 CPU/diffuse batch, FIFO, two-frame cap | 28,296 complete | 60.00 | 59.8 | 19 | Pass |
| Without coarse sleep, natural full story | 127,737 complete | 59.97 | 56.4 | 20 | Fail in dense Stage 4 |
| Without coarse sleep, Stage 3→4 diagnostic | 52,000 prefix | 59.96 | 57.8 | 21 | Fail; route incomplete |
| Stage 4 FIFO cap 2 plus coarse sleep | 28,296 complete | 60.00 | 59.8 | 19 | Pass; CPU time −27.1% |
| Same coarse-sleep profile, story | 84,694 prefix | 59.98 | 56.4 | 19 | Fail; stopped after irreversible timing failure |
| Mapped journals + early submission + eight workers, story | 127,737 complete | 59.990 | 58.4 | 18 | Pass both v4 clocks; complete native agreement |

The passing Stage 6 run peaks at 350,384,128 bytes with zero OOM events; its
24,536 precision entries all restore before gameplay. Steady output still contains
a 119-ms maximum gap and five gaps above 50 ms, retained in the report. The gate
does not certify that every frame is exactly 16.67 ms.

The earlier standalone Stage 4 profile peaks at 288,395,264 bytes (275 MiB), OOM 0. Its
28,277 CPU-draw entries all restore, and all 28,397 limiter entries restore.
The window report retains a 45-ms maximum gap, one gap above 33.3 ms and none
above 50 ms. All 28,296 native-projected updates match. The separate final normal-story case supplies its own six-clear terminal and
complete timing report.

The first full-story attempt fails inside Stage 4 around local update 21,800
(global updates 76,521–76,809). Draw submission is about 2 ms, while Present
repeatedly takes 20–28 ms. This is separate from loading gaps; the numerical gate
is unchanged. The stopped NMNB prefix matches all 91,553 native rows, peaks at
385,306,624 bytes and has zero OOM events. It is not a completed story.
Selected Stage 4 controls using two/six/eight workers without batching fail at
32.6/53.8/46.6 Hz over five seconds. More workers alone are not an improvement.
A four-worker control changing only `d3d9.maxFrameLatency` from 1 to 2 also fails
at 52 Hz. DXVK's
[frame-latency synchronization](https://github.com/doitsujin/dxvk/blob/v2.4/src/d3d9/d3d9_swapchain.cpp#L1127)
waits on completed frames at this cap, but a larger queue did not fix output.
DXVK 3.1.1 completes Stage 4 with matching native rows and zero misses/bombs, yet
fails both a cold shader stall (window maximum 4.822 seconds) and later dense
output (53.6-Hz API five-second diagnostic after update 1,000). It is not adopted.

Earlier Wine 8 vertex-color, CPU-transform and quad-batch experiments, limiter-sleep
variants and one/eight-thread controls also failed; those frozen implementations
remain private. The current CPU batch adapter is retested with the new WoW64/DXVK
backend and supports both source vertex formats. A two-thread GDB trial
was already failing before read-only profiling; it is diagnostic, not a timing
control. A timer API microbenchmark ruled out repeated resolution calls as a
meaningful cost (149 versus 112 ns per poll). Its cleanup timeout is retained.
Wine 11's [timer implementation](https://github.com/wine-mirror/wine/blob/wine-11.0/dlls/kernel32/sync.c#L838)
returns success for these 1-ms resolution requests without changing resolution;
no speculative permanent-resolution patch is added.

Three early renderer controls missed the systemd environment boundary and are
explicitly marked invalid. The builtin Vulkan control failed before its first
frame. DXVK can initialize Vulkan separately; that earlier assertion does not
establish that all Vulkan paths are unavailable. Earlier `reference-*` follower
fixtures predate pure-RPY acceptance and never count as playback successes.

## Artifacts and checks

| Artifact | Meaning |
|---|---|
| `retail-states.bin`, deferred `retail.csv`, `native-comparison.json` | All 15 typed projection fields; complete-route/prefix distinction |
| `update-times.bin` | Versioned header and three uint64 words per callback |
| `render-times.bin` | Versioned header and eight uint64 words per API presentation |
| `display-times.csv` | Raw XDamage server/receiver times, latest callback and geometry |
| `pacing.json` | Full/steady distributions, independent clocks and versioned gates |
| `clock.json`, `outcome.json` | Gameplay FPU mode, graphics options, natural result and precision counters |
| `effective-*-environment.json` | Worker and explicitly constructed Wine-child environments |
| `wine-runtime.json`, `graphics-libraries.json` | Runtime version, Unix bitness and actual loaded library hashes |
| `resource-receipt.json` | Scope, limits, identities, cache state, peak memory/OOM and host conditions |

```sh
python3 scripts/replay_pacing.py reports/local/replay-story
ctest --test-dir build --output-on-failure \
  -E 'real_data_contracts|story_transition_contracts|story_terminal_contracts|extra_bootstrap_contracts'
```

That replay-rendering milestone passed 26 focused contracts, including raw-vertex byte
order/winding and buffer bounds, the real x87 regression/restoration, integer timing,
synthetic 60/30-Hz discrimination, rejection of asynchronous API receipts without output
proof, malformed streams, selected native-route comparison and missing service
environment rejection. Actual retail cases remain necessary.

The full-story render/FPU entry and restoration counters balance, and unpaused
read-only process diagnostics confirm the batch adapter survives handoffs. The
worst retained window is global updates 76,720–77,000 in Stage 4. This is a timing
counterexample, not a gameplay divergence. Its Wine-tree peak is 362,463,232 bytes
with zero OOM events. The optional coarse-sleep profile passes the complete
Stage 4 and peaks at 295,067,648 bytes with zero OOM events. Its natural full
story fails the same dense interval and remains an incomplete 84,694-update
native-matching NMNB prefix. This rules out CPU spinning as a sufficient fix.

Source-guided follow-up compares selected-stage and natural-story drawing. Across
1,201 target frames, quad counts, all vertex bytes and screen coverage agree
exactly; sampled D3D render/texture/transform states also agree. DXVK 2.4's
[EndScene](https://github.com/doitsujin/dxvk/blob/v2.4/src/d3d9/d3d9_device.cpp#L1656)
uses a strong flush hint, while its
[flush tracker](https://github.com/doitsujin/dxvk/blob/v2.4/src/util/util_flush.cpp)
can defer fewer than three command chunks. An optional explicit submission at
EndScene is implemented by the opt-in bridge. The complete combined profile
passes; the separate controls do not prove a universal single-change speedup.

The [DXVK performance guidance](https://github.com/doitsujin/dxvk/wiki)
recommends checking drivers, synchronization, logging and compositor overhead.
The [OpenInputLagPatch author](https://github.com/khang06/OpenInputLagPatch)
documents a sleep-based limiter and a changed update/draw order for TH06/07.
Its waiting approach is useful prior art; adopting the entire patch would need
to respect this project's original callback-order contract. This host exposes no
DRI GPU device and uses software Vulkan. Desktop GPU/compositor recipes need
separate applicability checks; they are not evidence for this replay profile.

The opt-in [D3D8 source patch and private build](DXVK_REPLAY_BUILD.md) now have
same-DLL on/off controls. With eight workers, the retained 1,201-frame target
window improves from p99 21 ms to 19 ms; Present-call p99 falls from 13.70 to
6.75 ms. Both 78,000-update diagnostic prefixes match native. The normal full
story crosses that same window at slowest 5 s 59.4 Hz / p99 18 ms and completes
all six stages with full native agreement and both gates passing. Its private
case is `rpy-story-mapped-earlyflush-lp8-full001`. The acceptance receipt is
`reports/local/pacing-20261026/replay-pacing-acceptance.json`; it cross-checks
identities, counts, scopes, memory/OOM and raw artifact hashes. A raw-window
comparison plot is retained alongside it as `replay-pacing-comparison.png`.
