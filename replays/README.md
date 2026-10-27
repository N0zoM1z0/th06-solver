# Selected verified input-only replays

These owner-selected artifacts contain original-format replay metadata and input
changes, not EXE/DAT assets or native world snapshots. Copy them into the Japanese
1.02h game's `replay/` directory and select them from the replay menu.

| Replay | Route and origin | Seed | Complete updates | Bytes |
| --- | --- | ---: | ---: | ---: |
| [`th6_01.rpy`](th6_01.rpy) | Lunatic Reimu A, offline story, stage 1 / power 0 | 0 | 127,737 / six clears | 76,528 |
| [`th6_02.rpy`](th6_02.rpy) | Extra Reimu B, actual online shadow-MPC inputs, normal power 0 | 35964 | 43,126 / one clear | 41,784 |

Both pass whole-route original-loader Wine playback with zero misses/bombs and
agreement with the declared native projection. Export replays every source input
and checks every native digest before writing the `.rpy`; it does not replace
the Extra online route with a new offline search. The single Extra record uses
original replay stage slot seven, leaving the six main-game slots empty.

<a id="extra-online-nmnb-route"></a>

## File identities and provenance

| File | SHA-256 | Publication commit |
| --- | --- | --- |
| `th6_01.rpy` | `c29f3975d9b2744d3460c7946c7130bdc026dc3c47566c1e0d45bf7c14fb192d` | `387a7c58ea7e2347791fd1eeb1549e02785c73b3` |
| `th6_02.rpy` | `a64ec1d2ee7d74e2c22a818a6b6b2d793570eae4e39cd1e6063ca0e0f1184a3a` | `61c0eaefc46fd3df597e3748635967cf9117a7b1` |

The source profile is `native-i386-v1`, targeting Japanese 1.02h EXE SHA-256
`9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245`.
The story's native tape is `story-d3-shot0-seed0.actions`. The Extra source is
actual case `online-acceleration-reimuB-008`, with tape SHA-256
`98025c911e03c56225a83819ac3461eeb76d4b39fbeff3b77bab1f8acd9aee61`.
[Extra online evidence](../docs/EXTRA_SEARCH.md#first-complete-extra-online-nmnb-clear)
and [retail interoperability](../docs/RETAIL_REPLAY.md) explain their distinct proofs.

## Recordings and acceptance

Both selected files have full 640×480 compact Wine replay recordings with
original game audio. [Recording details](../docs/REPLAY_PACING.md#record-the-replay)
contain media checks, sizes, durations and private delivery paths; the
[Extra recording](../docs/REPLAY_PACING.md#extra-online-route-recording) also
retains its loader smoke and failed-monitor attempt.

Original gameplay, original replay-menu return, complete media decode, capture
timestamps and presentation pacing are separate checks. Both audio-on recordings
pass route/media validation and fail the unchanged pacing gate. The earlier
Lunatic audio-off pacing pass does not establish online FPS. Replays of online
inputs are not additional live controller trials or live-control footage.

These are the only tracked generated replay exceptions. Assets, other generated
replays, tapes, preparation files, recordings and raw attempt logs stay ignored.
