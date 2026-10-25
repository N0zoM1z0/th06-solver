# Third-party provenance

- `third_party/th06/src/` imports the source tree of
  [N0zoM1z0/th06](https://github.com/N0zoM1z0/th06) at
  `9e79eb8d784b6346233f2e98e1bcda1d2f04da15` (MIT, copyright 2026 N0zoM1z0).
  That reconstruction uses [GensokyoClub/th06](https://github.com/GensokyoClub/th06)
  at `7411374cef282756ef5a8984ce896fb8db797e9e` (CC0-1.0). Both licenses are
  retained in `third_party/th06/`. The import is tracked, not a submodule.
- Local TH06 changes adapt template/assert syntax, explicit x87 operand sizes,
  a rendering-only rounding block, VC7 loop scope, a diagnostic format string,
  and fail-closed native opcode dispatch. Source gameplay algorithms are retained.
- `src/native_support.cpp` carries the imported chain, RNG, timer and angle
  helpers, separated from Windows controller/filesystem/audio initialization.
- `third_party/th08-compat/` reuses Linux Win32/Direct3D declarations, CPU D3D
  resources and D3DX math from the tracked native TH08 source in
  [N0zoM1z0/th08-solver](https://github.com/N0zoM1z0/th08-solver), revision
  `1c68dc9639f5`. The original MIT license (2025 KSS; 2026 N0zoM1z0) is retained.
  TH06 adds missing declarations, uses the DirectX-8 image-info layout and redirects
  includes; no SDL/OpenGL implementation is imported.
- Imported DXUtil/DSUtil declarations retain their Microsoft copyright notices.

Touhou and its original game code/assets are by ZUN / Team Shanghai Alice. The
repository contains reconstruction and solver code only. Original EXE/DAT/image/
audio files and the development archive remain local and ignored.
