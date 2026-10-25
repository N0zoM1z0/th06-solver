# Wine retail replay interoperability

The current trial uses the complete seed-0 Lunatic Reimu A native story:
127,737 accepted updates, stage 1 / power 0 through the natural six-stage ending.
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
Use a new output directory for each attempt. All assets, replays, logs and runtime
snapshots remain under ignored `reports/local/`.

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

Initial probes: the first 3,000 updates match the native CSV in every recorded
field and have no miss/bomb. A later controlled infrastructure restart preserves
9,472 matching updates; that aborted attempt is not a full clear. The frozen full
story test is in progress. The combined native/retail tool tests pass 11/11.
The scalar comparator also has deterministic rejection tests for truncated rows,
unequal traces and incomplete/premature completion records.

Unsupported evidence claims: one successful retail story would not certify the
other 111 native cases, every seed, all Wine/renderer versions, or complete hidden
state equality. Logs and exact binary/replay identities identify the tested run.
