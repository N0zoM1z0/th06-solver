#include "th06/platform.hpp"
#include "TextHelper.hpp"
#include "stdafx.h"
#include "th06/resources.hpp"
#include <cstdarg>
#include <fstream>
#include <stdexcept>

namespace th06 {
u8 *FileSystem::OpenPath(const char *name, ZunBool external) {
    if (external)
        throw std::runtime_error(std::string("external game file unsupported: ") + name);
    const auto data = solver::resources().read(name);
    auto *out = static_cast<u8 *>(std::malloc(data.size()));
    if (!out)
        throw std::bad_alloc();
    std::memcpy(out, data.data(), data.size());
    g_ReadFileSize = data.size();
    return out;
}
int FileSystem::WriteDataToFile(const char *, const void *, size_t) {
    throw std::runtime_error("implicit game file writes are disabled");
}
namespace {
const char *diagnostic(const char *fmt, va_list args, bool fatal) {
    static char message[2048];
    vsnprintf(message, sizeof(message), fmt, args);
    if (fatal)
        throw std::runtime_error(message);
    std::fprintf(stderr, "%s", message);
    return message;
}
} // namespace
const char *GameErrorContext::Log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char *out = diagnostic(fmt, args, false);
    va_end(args);
    return out;
}
const char *GameErrorContext::Fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char out[2048];
    vsnprintf(out, sizeof(out), fmt, args);
    va_end(args);
    throw std::runtime_error(out);
}
BOOL Supervisor::LoadPbg3(i32, const char *) {
    return TRUE;
}
void Supervisor::ReleasePbg3(i32) {}
ZunBool Supervisor::ReadMidiFile(u32, const char *) {
    return true;
}
ZunBool Supervisor::PlayMidiFile(i32) {
    return true;
}
ZunResult Supervisor::PlayAudio(const char *) {
    return ZUN_SUCCESS;
}
ZunResult Supervisor::StopAudio() {
    return ZUN_SUCCESS;
}
ZunResult Supervisor::FadeOutMusic(float) {
    return ZUN_SUCCESS;
}
void SoundPlayer::PlaySoundByIdx(SoundIdx, i32) {}
void TextHelper_RenderTextToTexture(i32, i32, i32, i32, i32, i32, ZunColor, ZunColor, const char *,
                                    LPDIRECT3DTEXTURE8) {}
} // namespace th06

BOOL DeleteDC(HDC) {
    return TRUE;
}
HDC CreateCompatibleDC(HDC) {
    return reinterpret_cast<HDC>(1);
}

HRESULT D3DXCreateTexture(IDirect3DDevice8 *device, UINT w, UINT h, UINT levels, DWORD usage,
                          D3DFORMAT format, D3DPOOL pool, IDirect3DTexture8 **out) {
    return device->CreateTexture(w, h, levels, usage, format, pool, out);
}
// Pixel colors are presentation-only. Preserve dimensions, format and owned
// surfaces for the original ANM loader; no graphics or image libraries are used.
HRESULT D3DXCreateTextureFromFileInMemoryEx(IDirect3DDevice8 *device, LPCVOID data, UINT size,
                                            UINT width, UINT height, UINT levels, DWORD usage,
                                            D3DFORMAT format, D3DPOOL pool, DWORD, DWORD, D3DCOLOR,
                                            D3DXIMAGE_INFO *info, void *, IDirect3DTexture8 **out) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    std::int32_t w, h;
    if (size >= 26 && bytes[0] == 'B' && bytes[1] == 'M') {
        std::memcpy(&w, bytes + 18, 4);
        std::memcpy(&h, bytes + 22, 4);
    } else if (size >= 24 && std::memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) == 0) {
        auto be32 = [&](unsigned offset) {
            return (std::uint32_t(bytes[offset]) << 24) | (std::uint32_t(bytes[offset + 1]) << 16) |
                   (std::uint32_t(bytes[offset + 2]) << 8) | bytes[offset + 3];
        };
        w = be32(16);
        h = be32(20);
    } else
        return E_INVALIDARG;
    if (w <= 0 || h == 0 || h == INT32_MIN)
        return E_INVALIDARG;
    if (h < 0)
        h = -h;
    if (!width || width == D3DX_DEFAULT)
        width = w;
    if (!height || height == D3DX_DEFAULT)
        height = h;
    if (format == D3DFMT_UNKNOWN)
        format = D3DFMT_A8R8G8B8;
    if (info)
        *info = {UINT(w), UINT(h), 1, format, D3DRTYPE_TEXTURE};
    return D3DXCreateTexture(device, width, height, levels, usage, format, pool, out);
}
