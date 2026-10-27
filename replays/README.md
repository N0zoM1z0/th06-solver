# Verified input-only replays

[`th6_01.rpy`](th6_01.rpy) is the generated input-only replay for Lunatic Reimu A,
seed 0, starting at stage 1 / power 0. The owner explicitly requested that this
verified artifact be committed. It contains replay metadata and input changes,
not game assets. Copy it to the Japanese 1.02h game's `replay/` directory and
select it from the replay menu.

Wine retail playback reached the natural six-stage replay return at update
127,737 with zero misses/bombs. All 127,737 rows match the native trace's declared
15-field projection. The full-playback cgroup peak was 348,819,456 bytes under a
2-GiB memory limit with swap disabled and zero OOM events. This verifies this
specific story, not other seeds, shots or complete hidden-state equality.

- File size: 76,528 bytes.
- Replay SHA-256: `c29f3975d9b2744d3460c7946c7130bdc026dc3c47566c1e0d45bf7c14fb192d`.
- Original EXE SHA-256: `9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245`.
- Native tape: `story-d3-shot0-seed0.actions`, profile `native-i386-v1`.

The 2026-10-27 full Wine recording uses this exact replay SHA-256, already
published in commit `387a7c58ea7e2347791fd1eeb1549e02785c73b3`. Its independent
post-game comparison again matches all 127,737 updates and six natural NMNB
clears. The private recording is retained under
`reports/local/replay-video-20261027/story-compact001/`; the
[recording command](../docs/REPLAY_PACING.md#record-the-replay) produces a
640×480 original-speed compact MP4 with the game's WAV music and sound effects.
The verified compact delivery is 35:35.744 / 503,859,305 bytes; all 128,141 video
frames decode with the original capture timestamps. It is also hard-linked as
`reports/local/replay-video-20261027/th06-lunatic-reimuA-story-compact.mp4`.
Audio-on recording has separate pacing evidence from the earlier audio-off run.

[Retail verification](../docs/RETAIL_REPLAY.md) describes export, observation,
acceptance and the separately fixed Wine service cleanup issue. All original
EXE/DAT files, other generated replays, tapes and raw attempt logs stay ignored.


## Extra online NMNB route

[`th6_02.rpy`](th6_02.rpy) stores the actual online shadow-MPC clear's inputs:
Extra / Reimu B, retail seed 35964, natural power-0 start, 43,126 updates and
zero misses/bombs. The original live Wine clear and full independent native
replay are documented in [the Extra study](../docs/EXTRA_SEARCH.md#first-complete-extra-online-nmnb-clear).
Export replays the entire actual tape and checks every native digest; it does
not search for a replacement route. The original Extra record occupies replay
stage slot seven, with the six main-game slots empty. Copy the file into the
Japanese 1.02h game's `replay/` directory and select it in the replay menu.

- File size: 41,784 bytes.
- Replay SHA-256: `a64ec1d2ee7d74e2c22a818a6b6b2d793570eae4e39cd1e6063ca0e0f1184a3a`.
- Source actual tape SHA-256: `98025c911e03c56225a83819ac3461eeb76d4b39fbeff3b77bab1f8acd9aee61`.

The owner's requested compact recording uses actual Wine replay playback of
this file, separately from the original online decision-time evidence. Its
1,200-update loader/capture smoke test matches the fresh native export; full
playback and recording acceptance are reported separately when complete.
The owner authorizes this second tracked replay exception; assets, tapes,
recordings and raw evidence remain private.
