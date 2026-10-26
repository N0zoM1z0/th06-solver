# Experimental replay D3D8 bridge

This is a locally altered DXVK 2.4 D3D8 bridge, not an upstream DXVK release.
The source patch retains the zlib/libpng license and marks the DLL version
resource as a TH06 replay experiment. No original game source, EXE or DAT changes.
The new option defaults off. The complete normal-story timing result is pending.

Upstream: https://github.com/doitsujin/dxvk
Pinned revision: `008afc1c5f287e82a553108790da6e10b1159200` (`v2.4`).
Patch SHA-256: `8539f5ee9d9b1b05d16683d0f2429f71ddb732d476a73d427dae1a75b24fe689`.
Local marked D3D8 DLL SHA-256:
`f857d2ea50658320ebe7cf0b03870f178391a09be5f73a2ac047df0c06df0d1f`.
The D3D9 DLL remains the official DXVK 2.4 x32 binary.

## Mechanism

DXVK's D3D9 EndScene uses an implicit strong flush hint. Its tracker may defer
fewer than three command chunks. With `d3d8.earlyFlush = True`, the D3D8 bridge
forwards EndScene, then calls the supported ID3D9VkInteropDevice interface's
FlushRenderingCommands. It starts asynchronous GPU submission before TH06 waits
for its original deadline. The original update/draw callback order, vertex
commands, textures, Present, clock values and gameplay remain unchanged.

A separate translation unit uses DXVK's real SDK interface header, avoiding any
private device layout or guessed vtable offsets. The same DLL with the option
disabled supplies the comparison. Tests are finite and use one Wine tree under
a 2-GiB/no-swap cap. Accelerated diagnostic prefixes are never FPS acceptance.

## Build

Requires Meson, Ninja, an i686 MinGW POSIX C/C++ toolchain and glslangValidator.
Build privately; generated runtime binaries are not committed. On the measured
Debian 12 host, the compiler is GCC 12 and glslang-tools is 12.0.0-2. Tools were
extracted into a private directory instead of changing host packages.

```sh
git clone --branch v2.4 --depth 1 --recurse-submodules --shallow-submodules \
  https://github.com/doitsujin/dxvk.git /path/to/private-dxvk
# Check that HEAD equals the pinned revision above.
git -C /path/to/private-dxvk apply /path/to/dxvk-v2.4-early-flush.patch
meson setup /path/to/private-dxvk-build /path/to/private-dxvk \
  --cross-file /path/to/private-dxvk/build-th06-win32.txt --buildtype release \
  -Denable_d3d8=true -Denable_d3d9=true -Denable_dxgi=false \
  -Denable_d3d10=false -Denable_d3d11=false
systemd-run --user --wait --pipe -p MemoryMax=2G -p MemoryHigh=1536M \
  -p MemorySwapMax=0 -p TasksMax=256 -p RuntimeMaxSec=600 \
  ninja -C /path/to/private-dxvk-build -j2 src/d3d8/d3d8.dll
```

Keep the private tool directory in the service PATH if needed for Meson
regeneration. The D3D8 target links only its used utility and D3D9 import-library
dependencies; it does not build unused D3D9/Vulkan core objects. The measured
build takes 65 seconds under its own 2-GiB/no-swap limit. Clean patch application
and the paired private Wine probes pass. These checks do not establish a complete
story's FPS gate.

Copy the resulting x32 DLL to an owned private game copy alongside the official
DXVK 2.4 x32 D3D9 DLL, preserving the original game configuration and assets.
Add `d3d8.earlyFlush = True` to that copy's dxvk.conf. This setting requires the
patched bridge; stock DXVK 2.4 does not implement it. Use the replay runner's
explicit native DLL overrides and pinned 64-bit Wine/lavapipe runtime.
