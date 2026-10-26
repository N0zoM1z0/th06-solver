# Wine retail replay interoperability

For current render-clock measurements, selected-stage playback, the scoped
limiter precision fix and the 60-FPS gate, see [REPLAY_PACING.md](REPLAY_PACING.md).
Those tests run only the original `.rpy` playback; they launch no MPC or native
follower. The full-story interoperability result below predates that optimization.

The complete seed-0 Lunatic Reimu A story now passes retail playback:
127,737 updates, stage 1 / power 0 through six natural clears, zero misses/bombs,
and every row matching the declared 15-field native projection.
The native `TH06ACTIONS` tape is an input to export; it cannot be renamed into a
retail replay. Native coverage remains 96 standalone cases and 16 stories.

## Artifact and format

The preserved Japanese 1.02h EXE is 512,000 bytes with SHA-256
`9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245`.
Tests use a separate copy of the supplied EXE/DAT files, Wine 8.0, an isolated
32-bit prefix with Japanese ANSI code page 932, and Xvfb/software rendering.
The original disk EXE remains unchanged.

`th06_replay_export` initializes the native story again, checks the DAT identity
and every tape digest, and captures each stage's carried resources and pre-init
seed. It writes the source's version-0x102 `T6RP` header, file-relative stage
offsets, sparse eight-byte input changes, terminal sentinels, additive checksum
and rolling byte obfuscation. Per-stage records obey the original 0x69780-byte
recording bound. The decoder rejects malformed offsets, checksums and sequences.
Only complete five/six-stage standard stories are currently exported.

This distinguishes two important alternatives: merely renaming native records
fails the retail format contract; resetting each stage to practice power/RNG
would fail continuous replay. Real retail loading and comparison across a natural
stage transition test both the file representation and the stage boundary.

## Build and run

Install Wine with 32-bit support, Xvfb and `i686-w64-mingw32-g++`, in addition to
the native build dependencies. The optional observer is built independently with
MinGW; the native runtime stays a Linux i386 program.

```sh
cmake -S . -B build -DTH06_WINE_REPLAY=ON
cmake --build build -j 2
build/th06_replay_export --data game_exe_data/extracted/th06 \
  --tape reports/local/story/story-d3-shot0-seed0.actions \
  --output reports/local/retail/game/replay/th6_01.rpy \
  --trace reports/local/retail/native-lunatic.csv
python3 scripts/run_wine_replay.py --game reports/local/retail/game \
  --prefix reports/local/retail/wine-prefix --output reports/local/retail/runs/full002 \
  --updates 180000 --seconds 3000
python3 scripts/compare_retail_trace.py --native reports/local/retail/native-lunatic.csv \
  --retail reports/local/retail/runs/full002/retail.csv \
  --outcome reports/local/retail/runs/full002/outcome.json --require-complete
```

The game directory and dedicated prefix must already be prepared. Development
uses windowed, fixed-60-fps rendering with music/sound disabled. The runner requires
an existing 32-bit prefix; it never uses or edits the user's default Wine prefix.
Use a new output directory for each attempt. Assets, attempt replays, logs and
runtime snapshots remain under ignored `reports/local/`. The owner-requested
verified [`replays/th6_01.rpy`](../replays/th6_01.rpy) is the single published
input-only replay exception.

The runner takes an exclusive ownership lock and copies the launcher/observer into
the attempt directory before starting. Atomic build outputs and immutable attempt
copies keep later compilation from modifying a running test. A user-systemd unit
contains Wine, all descendants and Xvfb, with **MemoryMax=2 GiB, MemorySwapMax=0,
TasksMax=256**, a 1.5-GiB memory high watermark and a finite runtime. Software
rendering uses two worker threads. The receipt records cgroup memory peaks and
OOM counters. Stop/timeout cleanup targets only the owned unit and prefix.

## Observation and acceptance

The launcher verifies the EXE hash and injects only its newly created suspended
process. Hook signatures and complete instructions are checked before startup.
The observer automates the same replay-menu handoff as `MainMenu::ReplayHandling`;
the original EXE then validates the `.rpy`, restores its stage records and owns all
gameplay input, RNG, physics, collisions, stage transitions and rendering. No
gameplay state is corrected from the native trace.

An observer at calc priority 15 captures each actual game update, including the
original replay manager's dialogue restart loops. Its streamed CSV has 15 fields:
global/stage-local frame, stage, input, x/y, timeline, RNG seed/draw count, power,
misses, bombs, score, rank and player state. This projection is not a complete
hidden-state hash. Immediate source death state is a failure even before the
miss counter changes. A retail clear requires the natural final replay-return
event, all required stage clears and zero misses/bombs; a safe prefix does not pass.

The completed trial (`reports/local/retail/runs/full002/`) has:

| Check | Result |
|---|---|
| Native / retail / compared rows | 127,737 / 127,737 / 127,737 |
| First difference in the 15 recorded fields | None |
| Natural stage clears / final replay return | 6 / supervisor state 8 |
| Misses / bombs | 0 / 0 |
| Cgroup memory peak | 348,819,456 bytes (332.66 MiB) |
| Memory hard limit / swap limit | 2,147,483,648 bytes / 0 |
| OOM / OOM kills | 0 / 0 |

The replay is 76,528 bytes, SHA-256
`c29f3975d9b2744d3460c7946c7130bdc026dc3c47566c1e0d45bf7c14fb192d`.
The EXE's disk hash still matches the preserved original. Final comparison,
natural terminal and resource receipts are `comparison-final.json`,
`outcome.json` and `resource-receipt.json` in that attempt directory. A stage-6
retail screenshot is retained privately as `stage6.png`.

The older wrapper used for this full run exited with a service cleanup timeout
after its owned launcher exited 0; the complete game outcome and trace agreement
precede that teardown failure. The updated runner shuts down only its owned Wine
prefix before closing X. A separate latest-binary 3,000-update check (`cleanup003`)
matches every observed field, verifies the kernel memory/swap limits, stops at
the requested incomplete frame budget (exit 4), and cleans up without a timeout.
Its peak is 381,435,904 bytes, with zero OOM events. Both receipts remain intact.

Earlier probes include a matching 3,000-update prefix and a controlled
infrastructure restart after 9,472 matching updates. Those aborted/incomplete
attempts are not full clears, and their logs and costs remain recorded.
The combined native/retail tool tests pass 11/11. The scalar comparator includes
deterministic rejection tests for truncated rows, unequal traces and
incomplete/premature completion records.

Unsupported evidence claims: this successful retail story does not certify the
other 111 native cases, every seed, all Wine/renderer versions, or complete hidden
state equality. Logs and exact binary/replay identities identify the tested run.
