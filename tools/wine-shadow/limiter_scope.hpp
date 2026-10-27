#pragma once
#include "th06/retail_clock.hpp"
#include <windows.h>

namespace wine_shadow {
// Install from the first limiter call, after D3D creates its device but before
// the first menu callback. No clocks or drawing calls are sampled here.
class LimiterScope {
    using Scene = HRESULT(WINAPI *)(void *);
    using Present = HRESULT(WINAPI *)(void *, const RECT *, const RECT *, HWND, const RGNDATA *);
    inline static LimiterScope *instance_ = nullptr;
    void *device_ = nullptr;
    Scene begin_ = nullptr;
    Present present_ = nullptr;
    static HRESULT WINAPI begin_scene(void *device) {
        auto &self = *instance_;
        self.precision.restore();
        return self.begin_(device);
    }
    static HRESULT WINAPI present(void *device, const RECT *source, const RECT *target, HWND window,
                                  const RGNDATA *dirty) {
        auto &self = *instance_;
        self.precision.restore();
        return self.present_(device, source, target, window, dirty);
    }

  public:
    th06::retail::LimiterPrecision precision;
    unsigned sleeps = 0;
    bool install(void *device) {
        if (device_)
            return device_ == device;
        if (!device)
            return false;
        auto **table = *static_cast<void ***>(device);
        if (!table || !table[15] || !table[34])
            return false;
        DWORD protection;
        if (!VirtualProtect(table + 15, 20 * sizeof(void *), PAGE_READWRITE, &protection))
            return false;
        present_ = reinterpret_cast<Present>(table[15]);
        begin_ = reinterpret_cast<Scene>(table[34]);
        device_ = device;
        instance_ = this;
        InterlockedExchangePointer(table + 15, reinterpret_cast<void *>(present));
        InterlockedExchangePointer(table + 34, reinterpret_cast<void *>(begin_scene));
        DWORD ignored;
        return VirtualProtect(table + 15, 20 * sizeof(void *), protection, &ignored) != 0;
    }
};
} // namespace wine_shadow
