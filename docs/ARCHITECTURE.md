# Native profile and contracts

## Data flow and ownership

Private DAT bytes → checked PBG3/LZSS extraction → original loaded ECL/ANM/STD/MSG
programs → original ordered calc chain → native collision/terminal state → versioned
action tape → independently initialized checker process.

`Resources` owns compressed archives. Every source `OpenPath` receives its own
malloc-owned decoded buffer, matching the original free/rebase convention.
Archive lookup uses the basename and ASCII case normalization. Loaded programs
and all mutable actors belong to one game process. Candidate futures use fork's
copy-on-write memory; no manager is copied into an invalid pointer graph.

The source is a VC7/i386 program. A 64-bit import would require explicit serialized
offset tables and pointer rebasing throughout ECL, ANM, STD and MSG. The selected
32-bit Linux build instead preserves these layouts; the original size/alignment
assertions remain active. VC-specific template/assert syntax is adapted without
disabling layout verification. Existing x87 helpers retain their operations with
explicit operand widths where Clang requires them.

## Update boundary and bootstrap

`Session` owns one source game per process. A start supplies stage, difficulty,
shot, seed, power and standalone/story mode. Menu/high-score initialization is
replaced by a named checkpoint, with rank 16, source difficulty rank bounds,
two spare lives and three bombs. Text/ASCII resources load before stage, player,
bullet, enemy, ECL, effect and GUI initialization. The gameplay RNG is seeded
once at bootstrap; future draws, including ANM/effects, use the original stream.
Extra selects the source's final score-extend table entry, disabling score-based
extra lives while retaining actual item effects and its rank bounds of 14–18.

Each supplied input runs the original calc chain once: ASCII priority 1, game
manager 4, stage 6, player 7, enemy 9, effects 10, bullets/items 11, GUI 12 and
screen effects 14. Bomb/menu input is rejected. Shoot, focus, movement and dialogue
skip use the source's normal input tests. Source death state is a failure even
before a delayed life counter could conceal it. Unknown ECL/ANM/timeline opcodes
throw and cannot certify a prefix.
Invalid input is rejected before a pending story transition can mutate resources,
RNG or the current stage.

Rendering and text rasterization are omitted. CPU Direct3D resources retain
dimensions, formats and lifetimes; PNG/BMP dimensions are read, while pixel colors
are not decoded. ANM/GUI update scripts and RNG consumption still execute. This
profile is an operational native simulator; retail-equivalent floating point,
menu entry and draw-chain behavior are not established.
Cross-runtime acceptance concerns gameplay progress and correctness. Harmless
numeric drift from Wine is diagnostic; bit-identical floating point is not a
requirement. Native parent/child and fresh-replay consistency remain internal
checks on deterministic execution.

## Measured native fast paths

Two headless-only fast paths retain source layout, arithmetic, clocks and RNG.
The ANM alpha loop skips a component when both byte endpoints are identical:
the finite integer-timer fraction makes that component an exact identity.
Changed components still use the original interpolation and truncation.

`BombProjectileScope` scans the source's `size.x == 0` region predicate once
after item update in the bullet callback. The remaining bullet/laser loop reads
those regions; player/BombData writes happen in the earlier player callback.
An owner-bound, stack-scoped empty result skips only the redundant region scan
in collision/graze queries. Active regions and unscoped/other-player calls use
the original loop, including bomb priority over player collision. Player update
invalidates matching enclosing scopes before any region writes; no scope survives
a native update or planner fork boundary. Debug builds recheck empty regions.

`TH06_FAST_ANM_COLORS` and `TH06_FAST_BOMB_REGIONS` default on and can be disabled
independently for measured ablations. See [Performance](PERFORMANCE.md) for the
source analysis, unchanged-input timing workloads and focused seed comparison.

## Planning and proof boundaries

`Observation` contains owned scalar hazard/movement data and no native pointers.
Cheap linear forecasts rank 33 deterministic candidates (focus/unfocused movement,
short movement followed by holding, or continued movement). This ranking omits
future spawns, non-linear transforms and laser geometry. It proposes actions only.
The default search evaluates four ranked proposals, expanding through the rest
when needed. The report records budgets and every actual branch/update cost.

Opt-in `--reactive` adds a continuation for each initial input. After the initial
commit-sized action, this branch steers every 16 native updates using owned future
observations and a cheap local forecast. That forecast chooses inputs only: all
movement, RNG, spawns and collisions still run in the source game. The parent
commits only the initial digest-checked action, then replans. Reactive branch
counts and their complete native update costs are recorded separately.

Opt-in `--diverse-first` evaluates the best-ranked future for each distinct
initial input before duplicate futures for that input. Short, constant and
reactive tails certify the same initial action, so a small proposal budget can
otherwise cover only a few initial inputs. This changes pruning/order, retains
the same native acceptance oracle, and has measured tradeoffs across stages;
it is not enabled by default. `unique_inputs`, `decisions` and
`short_horizon_decisions` expose search coverage. A short selected future can
also be a natural terminal truncation, so that counter alone is not a failure.

Each evaluated child runs the original game, including action-dependent targeting,
RNG, transforms, enemy bodies and lasers. It returns a consecutive safe prefix
and observable digests. The parent accepts only matching native execution, commits
a bounded prefix, and replans. Failed child processes are errors, not safe routes.
An exhausted frame budget writes an incomplete tape and exits 2.
The accepted frontier is also saved atomically every 600 updates, so a later
rollout error retains a replayable prefix; no speculative future is checkpointed.

For recovery, `th06_plan --prefix TAPE --prefix-frames N --exclude-first MASK`
initializes a fresh game and checks every prefix digest before branching from N.
The next action can exclude the earlier choice; all later prefixes still require
native validation. This is deterministic replay-based backtracking, without
teleporting actors or restoring incomplete manager snapshots. Prefix replay work
is reported separately. Campaign binaries are copied into immutable, identified
snapshots before tasks start; resuming uses fresh replay under the selected build.

`--profile` adds a separate `timings` object with parent/child CPU and phase
wall times. Child update/guidance/scoring/digest times are subsets of rollout
wall time, not additional elapsed time. The native profile identifier retains
its usual string type. `th06_runtime_bench DATA TAPE` measures native updates
without initialization, hashing or file I/O, in one fresh game process. Its
terminal digest check is a timing-workload guard; complete replay still uses
the checker and every per-frame digest.

Per-frame digests hash named player/RNG/rank/score/timeline and active actor,
bullet/laser/player-shot observations, never padding or pointers. They detect
observable native replay divergence, not complete hidden-state equality or retail
physics equivalence. DAT identity uses a documented non-cryptographic FNV-1a
fingerprint over archive bytes; stored PBG3 checksums validate extraction.

## Story lifecycle

Standalone checkpoints run the source's practice terminal path. Story mode runs
the normal MSG stage-end path. A pending NEXT_STAGE transition is processed at
the next input boundary, corresponding to the supervisor's next priority-0 call.
Stage/player/enemy/bullet/effect/GUI cut/register callbacks retain their source
order and observe NEXT_STAGE throughout the handoff.

RNG is never reseeded at a handoff; its source generation counter resets where
the original stage setup resets it. Rank, resources, cumulative statistics and
score handoff persist. The source resets stage actors, bullet/item pools, player
spawn state and stage counters. Player/GUI shared ANMs survive where their
callbacks test NEXT_STAGE. Easy takes the source's stage-5 ending; the other
standard difficulties end after stage 6. A later standalone clear cannot substitute
for a continuous-story replay.
Story mode fixes the required stage count from its initial difficulty and rejects
any terminal event before the required final stage and natural clear count.

## Discriminating probes

The initial resource hypothesis was a single archive namespace. Real archives
disproved unique membership (`ver0102.dat` appears in several); identical decoded
duplicates are now accepted and differing copies fail. PNG resources similarly
required a separate image-header path after the initial BMP probe failed.

Fixed versus moved stage-1 inputs produce different hit frames and RNG histories;
shared frozen futures would therefore be an invalid oracle. Independent seeded
stage starts and fresh-process replay are the additional predictions tested beyond
the first fitted stationary trace. Full stage coverage and story lifecycle fidelity
remain governed by the explicit acceptance matrix in Status.
