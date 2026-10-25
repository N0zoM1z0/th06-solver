// Adapted from th08-solver; MIT, copyright 2026 N0zoM1z0.
#pragma once
// Wincrypt requires the Windows scalar types before its declarations.
// clang-format off
#include <windows.h>
#include <wincrypt.h>
// clang-format on
#include <array>
#include <cstdio>
#include <string>

namespace retail_probe {
inline constexpr const char *retail_sha256 =
    "9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245";
inline bool pinned_executable(const char *path) {
    HANDLE file =
        CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    bool ok =
        GetFileSize(file, nullptr) == 512000 &&
        CryptAcquireContextA(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash);
    std::array<BYTE, 16384> buffer;
    DWORD size = 0;
    while (ok) {
        ok = ReadFile(file, buffer.data(), buffer.size(), &size, nullptr);
        if (!ok || !size)
            break;
        ok = CryptHashData(hash, buffer.data(), size, 0);
    }
    std::array<BYTE, 32> digest{};
    DWORD digest_size = digest.size();
    ok = ok && CryptGetHashParam(hash, HP_HASHVAL, digest.data(), &digest_size, 0) &&
         digest_size == digest.size();
    std::string hex;
    for (auto byte : digest) {
        char pair[3];
        std::snprintf(pair, sizeof(pair), "%02x", unsigned(byte));
        hex += pair;
    }
    if (hash)
        CryptDestroyHash(hash);
    if (provider)
        CryptReleaseContext(provider, 0);
    CloseHandle(file);
    return ok && hex == retail_sha256;
}
} // namespace retail_probe
