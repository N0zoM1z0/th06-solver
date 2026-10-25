# Lunatic random-seed checks

The current scope is Lunatic only. The fixed, public experiment definition is
[`reports/lunatic-random-20261025.json`](../reports/lunatic-random-20261025.json).
Four distinct nonzero 16-bit seeds were sampled from OS randomness before trials:
**5341, 7589, 30826, 9674**. Failed samples will not be replaced.

Each seed covers all six stages and four shots (96 standalone cases total), then
all four continuous six-stage stories (16 stories). Standalone powers are 0/64/128
for stage 1/2/3–6; stories start at power 0 and carry actual resources and RNG.
The historical seed-0 controls remain 24/24 Lunatic stages and 4/4 Lunatic stories.

## Contract and oracle

| Expectation | Observable oracle | Failure interpretation |
|---|---|---|
| Every selected seed covers exactly six Lunatic stages and four shots | An independently constructed case-name set, including checkpoint power | Wrong difficulty, omitted seed or duplicate coverage |
| A claimed clear reaches a natural terminal with no miss/bomb | Source terminal callback and immediate player death state | Runtime failure, unsafe route or incomplete search |
| The whole accepted route reproduces from its initial seed | A new checker process executes every action and checks every digest | Replay mismatch; not a pass |
| A story retains its seed/resource lineage through all six stages | Required natural clear count and complete fresh story replay | A later checkpoint cannot substitute for a continuous story |

The replay checker is independent of the planner's return status and proposals,
but shares the native source profile. These random-seed checks are native evidence,
not new retail equivalence claims or a proof for every 16-bit seed.

Ranked probes target three relevant seams: changed RNG can invalidate a short
lookahead route; actual story pickups/handoffs differ from practice checkpoints;
and a mistaken difficulty/seed selector can produce false coverage. The selection
regression constructs the expected 56-case set for seeds 17 and 65535 independently
of the scheduler. Controlled injection that ignores `--difficulty` makes it fail;
the real selector passes. Duplicate, empty, out-of-range and mixed seed arguments
are also rejected. The existing asset-free contracts pass 7/7.

## Fixed budgets and retention

Initial searches use reactive futures, horizon 120, commit 8 and six proposals.
The frame budgets are 60,000 per stage and 180,000 per story. At most two recovery
attempts are predeclared: rewind 768 / horizon 180 / eight proposals, then rewind
2,048 / horizon 240 / twelve proposals. Both retain commit 8 and reactive futures.
Only a digest-checked accepted prefix can be reused; all earlier attempts remain
in the ledger. Initial and recovered pass rates must be reported separately.
Budget exhaustion is not proof of unsolvability.

The entire campaign process group is capped at 2 GiB, swap disabled, with at most
four solver jobs. Raw tapes, retries, stdout/stderr and memory/OOM receipts stay
under `reports/local/lunatic-random-20261025/`. Public results will preserve the
selected seeds and account for failures rather than replacing samples.

```sh
build/th06_campaign --data game_exe_data/extracted/th06 --suite stages \
  --difficulty 3 --seeds 5341,7589,30826,9674 --jobs 4 --reactive \
  --horizon 120 --commit 8 --proposals 6 --frames 60000 \
  --output reports/local/lunatic-random-20261025/stages --resume
```

The measured run uses this command inside a user-systemd unit with `MemoryMax=2G`,
`MemoryHigh=1536M`, `MemorySwapMax=0`, `TasksMax=256` and a finite runtime.
## Standalone result

All **96/96 sampled stage cases** complete NMNB and pass full fresh-process
native replay. Initial search passes **92/96**; the first predeclared recovery
clears three more and the second clears the final case. No sample was replaced.

| Seed | Initial clears | Final clears |
|---|---:|---:|
| 5341 | 23/24 | 24/24 |
| 7589 | 23/24 | 24/24 |
| 30826 | 23/24 | 24/24 |
| 9674 | 23/24 | 24/24 |

| Stage-5 case | Initial safe frontier | Final updates | Final horizon |
|---|---:|---:|---:|
| Seed 5341, Marisa B | 10,641 | 19,554 | 180 |
| Seed 7589, Reimu B | 10,567 | 20,326 | 240 |
| Seed 30826, Marisa A | 10,586 | 21,846 | 180 |
| Seed 9674, Reimu A | 10,725 | 18,841 | 180 |

The seed-5341 counterexample is minimized to one search decision after replaying
its 10,641 accepted updates: all 50 evaluated continuations fail on their first
simulated update and the output frontier remains byte-identical. The same
stage/shot/power/search parameters with seed 0 complete at update 21,825 and pass
fresh replay. The incomplete random prefix itself passes replay but is rejected
by `--require-complete`; forging its completion flag is also rejected. This
demonstrates a seed-dependent local search trap and a working completion guard,
not an unsolvable stage. Rewinding to an earlier verified state finds a clear.

The sampled stage search ledger includes **101 attempts**, 1,721,318 native
branches, 171,990,949 speculative updates and 49,297 prefix-replay updates.
Complete routes contain 1,983,722 accepted updates. Summed per-attempt planning
wall time is 14,332.79 seconds across parallel jobs; it is not campaign elapsed
time and excludes separately recorded diagnostic/control work. Cgroup peak is
127,803,392 bytes (121.88 MiB), with zero OOM events under the 2-GiB/no-swap cap.

[Per-case stage evidence](../reports/lunatic-random-20261025-stages.csv) includes
all retry effort and tape SHA-256 identities. The 16 continuous-story cases are
still in progress. These are fixed-sample native results; the seed-0 baseline
does not count toward the sample and other 16-bit seeds remain untested.
