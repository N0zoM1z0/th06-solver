# TH06 solver engineering rules

Read README.md and docs/STATUS.md before changing behavior. Current gameplay
work focuses on live Extra shadow MPC in the owned Wine retail game, using
th08-solver as a reference. Measure actual decision deadlines, queue behavior
and presentation. The selected Extra clear/replay/recording is complete; random-
seed robustness and stable online 60 FPS remain open. Lunatic coverage is limited
to the documented live components and historical native/replay campaigns.

Keep native search, actual online control, original-loader replay and recording
evidence separate. During pure replay measurements/recording, run no MPC or
native follower. Playback FPS does not certify online FPS. Cross-runtime
comparison judges gameplay progress/correctness, not bit-identical floating
point; native fork/replay consistency remains an exact internal guard.

Do not rerun the full matrix or story campaign for routine optimization. Retain
the historical seed-0 matrix and all failed retries; do not schedule other
difficulties without a new scope. Use one Wine instance at a time under a 2-GiB
hard / 1.5-GiB high / no-swap cgroup. The owner explicitly permits a 4-GiB hard /
3-GiB high cap for the Extra replay recording. Record actual limits, peaks and
OOM counters. Keep the original EXE/DATs and all preparation artifacts private.

- Use C++17. Preserve the imported source, licenses and pinned provenance in-tree.
  Keep parsing, native state, proposals, process isolation, tapes and CLI separate.
- Native game rollouts decide collisions and terminal outcomes. Cheap proposal
  approximations cannot certify a route. Unknown gameplay opcodes must fail.
- Never memcpy pointer-rich managers as independent worlds. Forks own mutable
  memory, actor pointers and gameplay RNG. Preserve original callback order.
- Stage transitions must use the source's lifecycle rules: carry RNG, rank and
  resources; reset actors and pools only where the source does. No reseeding at
  a story handoff. Document checkpoints separately from natural story starts.
- Clear means the native terminal event, zero misses/bombs, and a fresh-process
  replay with matching per-frame observable digests. Prefixes and budget exhaustion
  must remain incomplete. Native agreement does not establish retail equivalence.
- Keep all EXE/DAT/assets/archives and preparation notes out of Git. Keep generated
  tapes, logs and campaign reports under ignored reports/local/ or build/.
  The owner explicitly requested publication of the verified input-only replay
  at replays/th6_01.rpy and the verified Extra online-clear input replay at
  replays/th6_02.rpy; these are the only tracked replay exceptions.
- Run focused regressions and CTest after behavioral changes; run affected real
  DAT cases when available. Preserve failed evidence and include all retries in
  campaign accounting. Use at most four concurrent solver jobs on this shared host.
- Keep documentation compact and current. Use English code comments and docs.
  Format first-party C++ with .clang-format; minimize changes to imported source.
- Publish tested progress on main without rewriting unrelated history. Commit
  subjects use `gpt-6.1-sol: <English summary>`.
