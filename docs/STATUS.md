# Native runtime status

Current scope: make the native runtime dependable for later shadow MPC. First
complete legitimate standalone stage/difficulty/shot combinations, then continuous
stories. No retail execution or retail replay work is in scope yet.

## Acceptance matrix

- Easy: stages 1–5, four shots (20 cases).
- Normal, Hard and Lunatic: stages 1–6, four shots each (72 cases).
- Extra: stage 7, four shots (4 cases).
- Story: four standard difficulties × four shots (16 cases). Easy ends after
  stage 5 as in the source; the other stories end after stage 6.

The initial seed is 0. Standalone checkpoints use power 0 at stage 1, 64 at
stage 2, and 128 at stages 3–7. Story starts at power 0 and carries actual pickups.
Additional seed coverage is separate from the matrix above.

A case passes only on the native terminal transition with zero misses/bombs and
a matching complete replay in a fresh process. A planned prefix, timeout, runtime
error or failed replay does not pass. All logs/tapes/reports stay in reports/local/.

## Verified evidence

Development environment: Linux x86-64, Clang 14.0.6, native 32-bit build, no
fast-math. The original EXE SHA-256 is
`9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245`.

- All six DAT archives parse; all members decompress and pass stored checksums.
  There are 337 distinct resource names; identical shared version members are
  accepted and conflicting duplicate content is rejected.
- First baseline: stage 1, Lunatic, Reimu A, seed 0, power 0. Fixed shoot/focus/skip
  input dies at update 319. Two fresh processes produce identical complete traces.
- First full solve: the same configuration completes at update 13,701 with zero
  misses/bombs. The exhaustive 33-proposal planner used horizon 90, commit 8,
  56,529 native branches and 4,355,906 simulated updates in 218.40 seconds.
  A new checker process replays every accepted update and confirms completion.
  The local tape SHA-256 is
  `4e575d68add3a8d412275791456877218fa0cc5df99da49c9f870cbe8b374138`.
- Initialization and live collisions have been exercised in stages 1–7 using
  Marisa B, seed 65535 and power 128. These short negative controls are not clears.
- Tests cover PBG3 truncation/checksums/overlapping dictionary references, tape
  sequencing and corruption, source RNG ordering, collision edges, unsupported
  opcodes and a planned prefix with fresh-process replay and rejection controls.

The full 96-case standalone matrix and 16-story matrix are in progress. Lunatic
stage 1 has cleared with all four shots. Additional complete, fresh-replayed
Lunatic stage-2 and stage-3 Reimu-A runs use power 128; the canonical stage-2 matrix
uses power 64 and is tracked separately. A ranked four-proposal run reproduces
the stage-1 completion in 26.80 seconds with 6,967 native branches and 603,011
simulated updates. All retry effort remains in the local attempt ledger.

Eight focused tests pass, including a source-binary replacement regression for
campaign snapshots, RNG/resource carry across a stage handoff, atomic rejection
of invalid handoff input, and Extra's source bootstrap/score-extend rule. A clean Release
build passes the asset-free contracts and replays the initial complete tape.
The first 96-case sweep completed 89 routes; remaining incomplete routes are being
repaired and all completions are rechecked with the current binary.

Extra Reimu B now completes at update 59,837 with zero misses/bombs and passes
fresh replay under the corrected runtime. Verified-prefix rewind first crossed
the update-13,078 dead end with horizon 180/proposals 8, then the update-47,943
dead end with horizon 240. Source review found the omitted Extra score-extend
disable; the corrected bootstrap selects source score-table index 4. Its first
observable difference occurred at update 58,556, so the route was resumed from
the checked prefix ending at 58,555 and fully replayed again. All earlier attempts
and their simulation/replay costs remain recorded; the last continuation's timing
is not the cost of the entire solve. Other Extra shots still require current-build
verification or repair. No full continuous story has yet been certified.

Each campaign snapshots its planner/checker binaries and records their identities.
Recompiling the workspace cannot change a running campaign. Resume rechecks each
existing completed tape in a new process before counting it. Startup failures from
the initial unsnapshotted run remain recorded as infrastructure failures, not
gameplay counterexamples or silently discarded attempts.
