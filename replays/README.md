# Verified retail replay

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
Audio-on recording has separate pacing evidence from the earlier audio-off run.

[Retail verification](../docs/RETAIL_REPLAY.md) describes export, observation,
acceptance and the separately fixed Wine service cleanup issue. All original
EXE/DAT files, other generated replays, tapes and raw attempt logs stay ignored.
