# TH06 solver engineering rules

Read README.md and docs/STATUS.md before changing behavior. The owner's immediate
scope is stable 60-FPS Wine retail playback of native-generated Lunatic .rpy
files, using th08-solver as a reference. Do not use online MPC or a native
follower during this pacing acceptance. Verify the original replay loader,
actual presentation clocks and native-export trace agreement. Online shadow MPC
and its decision/queue optimization remain separate later work; replay FPS
does not certify online FPS. Keep offline, playback and live evidence separate.
Do not rerun the full matrix or story campaign for routine optimization.
Retain the historical seed-0 matrix; do not schedule other difficulties now.
The owner has now authorized retail replay interoperability under Wine, starting
with a complete Lunatic story. Keep native and retail proof separate. Run one
Wine instance at a time under a 2 GiB cgroup memory cap with swap disabled.
The owner also authorizes an explicit 4 GiB cap for the Extra replay recording; retain its
actual limit/peak receipt and keep swap disabled.
Cross-runtime comparison should judge gameplay progress/correctness; exact
floating-point identity with Wine is not required. Native fork/replay consistency
remains a useful internal guard.

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
