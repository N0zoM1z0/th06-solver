# Native runtime status

Current scope: make the native runtime dependable for later shadow MPC.
The complete standalone matrix passes 96/96, including fresh native replay of
every entire route. The 16 continuous stories are in progress. No retail execution
or retail replay work is in scope yet.

## Acceptance matrix

| Suite | Domain | Complete fresh-replayed NMNB cases |
|---|---|---:|
| Easy | Stages 1–5, four shots | 20/20 |
| Normal | Stages 1–6, four shots | 24/24 |
| Hard | Stages 1–6, four shots | 24/24 |
| Lunatic | Stages 1–6, four shots | 24/24 |
| Extra | Stage 7, four shots | 4/4 |
| Continuous story | Four standard difficulties × four shots | In progress |

Easy story ends after stage 5 as in the source; the other stories end after stage 6.
An earlier terminal event is rejected, even if the current route has no misses.

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

The complete 96-case standalone matrix uses the canonical checkpoints above.
Earlier standalone stage-2 power-128 experiments are separate evidence; the
matrix stage-2 checkpoint uses power 64. A ranked four-proposal run reproduces
the stage-1 completion in 26.80 seconds with 6,967 native branches and 603,011
simulated updates. All retry effort remains in the local attempt ledger.

Nine focused tests pass, including a source-binary replacement regression for
campaign snapshots, RNG/resource carry across a stage handoff, atomic rejection
of invalid handoff input, Extra's source bootstrap/score-extend rule, reactive
future continuation replay, and rejection of a premature story terminal. A clean Release
build passes the asset-free contracts and replays the initial complete tape.
The final standalone gate independently replays all 96 complete tapes under the
current runtime and reports 96 cleared, 0 failed. Earlier incomplete searches and
their costs remain in the attempt ledger.

Extra Reimu B now completes at update 59,837 with zero misses/bombs and passes
fresh replay under the corrected runtime. Verified-prefix rewind first crossed
the update-13,078 dead end with horizon 180/proposals 8, then the update-47,943
dead end with horizon 240. Source review found the omitted Extra score-extend
disable; the corrected bootstrap selects source score-table index 4. Its first
observable difference occurred at update 58,556, so the route was resumed from
the checked prefix ending at 58,555 and fully replayed again. All earlier attempts
and their simulation/replay costs remain recorded; the last continuation's timing
is not the cost of the entire solve. Reactive native futures then complete the
remaining Extra shots. Their current complete-route evidence is:

| Shot | Complete native updates | Last continuation horizon / proposals | Reactive futures |
|---|---:|---|---|
| Reimu A | 47,240 | 240 / 8 | Enabled |
| Reimu B | 59,837 | 240 / 8 | Disabled |
| Marisa A | 55,702 | 240 / 8 | Enabled |
| Marisa B | 61,696 | 320 / 12 | Enabled |

Every entry has a complete new-process replay with zero misses/bombs. The last
continuation budgets above do not replace earlier checked prefixes or erase retry
costs. No full continuous story has yet been certified.

Each campaign snapshots its planner/checker binaries and records their identities.
Recompiling the workspace cannot change a running campaign. Resume rechecks each
existing completed tape in a new process before counting it. Startup failures from
the initial unsnapshotted run remain recorded as infrastructure failures, not
gameplay counterexamples or silently discarded attempts.
