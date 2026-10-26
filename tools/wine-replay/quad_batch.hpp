#pragma once
#include "th06/retail_quads.hpp"
#include <cstdint>
#include <cstring>
#include <windows.h>

namespace wine_replay {
// Restrict deferred submissions to the original BulletManager draw callback:
// its Anm/Item draw paths never upload or mutate texture resources. Every other
// device method flushes first, so state changes, queries and unsupported draws
// retain their original ordering. This class never holds gameplay objects.
class QuadBatch {
    using Draw = HRESULT(WINAPI *)(void *, unsigned, unsigned, const void *, unsigned);
    using Shader = HRESULT(WINAPI *)(void *, DWORD);
    using GetShader = HRESULT(WINAPI *)(void *, DWORD *);
    inline static QuadBatch *instance_ = nullptr;
    void *device_ = nullptr;
    void *methods_[97]{};
    Draw draw_ = nullptr;
    Shader shader_ = nullptr;
    GetShader get_shader_ = nullptr;
    th06::retail::Quads quads_;
    th06::retail::FactorQuads factor_quads_;
    DWORD fvf_ = 0;
    bool active_ = false, failed_ = false;
    static void __attribute__((force_align_arg_pointer)) barrier() {
        instance_->flush();
    }
    template <class Vertices> void submit(Vertices &vertices) {
        if (!vertices.count)
            return;
        const auto count = vertices.count;
        vertices.count = 0;
        // D3DPT_TRIANGLELIST = 4. No arithmetic touches vertex values.
        failed_ = FAILED(draw_(device_, 4, unsigned(count * 2), vertices.vertices.data(),
                               unsigned(Vertices::stride))) ||
                  failed_;
        ++submitted;
        copied_quads += count;
    }
    void flush() {
        // Every FVF change flushes first, so only one format can be pending.
        if (quads_.count && factor_quads_.count)
            failed_ = true;
        submit(quads_);
        submit(factor_quads_);
    }
    static HRESULT WINAPI shader(void *device, DWORD value) {
        auto &self = *instance_;
        self.flush();
        const auto result = self.shader_(device, value);
        if (device == self.device_ && SUCCEEDED(result))
            self.fvf_ = value;
        return result;
    }
    static HRESULT WINAPI draw(void *device, unsigned type, unsigned count, const void *vertices,
                               unsigned stride) {
        auto &self = *instance_;
        ++self.draw_calls;
        const bool diffuse = self.fvf_ == 0x144 && stride == th06::retail::Quads::stride;
        const bool factor = self.fvf_ == 0x104 && stride == th06::retail::FactorQuads::stride;
        if (self.active_ && device == self.device_ && (diffuse || factor) && type == 5 &&
            count == 2 && vertices) {
            if (self.quads_.count == th06::retail::Quads::capacity ||
                self.factor_quads_.count == th06::retail::FactorQuads::capacity)
                self.flush();
            if (!(factor ? self.factor_quads_.append(vertices) : self.quads_.append(vertices))) {
                self.failed_ = true;
                return E_FAIL;
            }
            return S_OK;
        }
        self.flush();
        return self.draw_(device, type, count, vertices, stride);
    }

  public:
    std::uint64_t draw_calls = 0, submitted = 0, copied_quads = 0, scopes = 0;
    bool active() const {
        return active_;
    }
    bool failed() const {
        return failed_;
    }
    bool enter() {
        if (active_ || FAILED(get_shader_(device_, &fvf_)))
            return false;
        active_ = true;
        ++scopes;
        return true;
    }
    void leave() {
        flush();
        active_ = false;
    }
    bool install(void *device) {
        static_assert(sizeof(void *) == 4);
        if (device_)
            return device_ == device;
        if (!device)
            return false;
        auto **original = *static_cast<void ***>(device);
        draw_ = reinterpret_cast<Draw>(original[72]);
        shader_ = reinterpret_cast<Shader>(original[76]);
        get_shader_ = reinterpret_cast<GetShader>(original[77]);
        // Each i386 thunk preserves the incoming flags/registers/stack, calls a
        // no-argument flush barrier, then tail-jumps to the unchanged SDK method.
        // No unknown method signature or argument count is guessed.
        constexpr unsigned size = 14;
        auto *code = static_cast<unsigned char *>(
            VirtualAlloc(nullptr, 97 * size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        if (!code)
            return false;
        for (unsigned i = 0; i < 97; ++i) {
            auto *p = code + size * i;
            const unsigned char prefix[] = {0x9c, 0x60, 0xe8};
            std::memcpy(p, prefix, sizeof(prefix));
            auto relative = std::uint32_t(reinterpret_cast<std::uintptr_t>(barrier) -
                                          reinterpret_cast<std::uintptr_t>(p + 7));
            std::memcpy(p + 3, &relative, sizeof(relative));
            p[7] = 0x61;
            p[8] = 0x9d;
            p[9] = 0xe9;
            relative = std::uint32_t(reinterpret_cast<std::uintptr_t>(original[i]) -
                                     reinterpret_cast<std::uintptr_t>(p + size));
            std::memcpy(p + 10, &relative, sizeof(relative));
            methods_[i] = p;
        }
        DWORD previous;
        if (!VirtualProtect(code, 97 * size, PAGE_EXECUTE_READ, &previous) ||
            !FlushInstructionCache(GetCurrentProcess(), code, 97 * size)) {
            VirtualFree(code, 0, MEM_RELEASE);
            return false;
        }
        methods_[72] = reinterpret_cast<void *>(draw);
        methods_[76] = reinterpret_cast<void *>(shader);
        device_ = device;
        instance_ = this;
        InterlockedExchangePointer(reinterpret_cast<void **>(device), methods_);
        return true;
    }
};
} // namespace wine_replay
