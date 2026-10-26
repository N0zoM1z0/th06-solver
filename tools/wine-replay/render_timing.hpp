#pragma once
// Adapted from th08-solver 1c68dc9639f5 (MIT, N0zoM1z0).
#include "mapped_journal.hpp"
#include "th06/render_timing.hpp"
#include <cstring>
#include <windows.h>

namespace wine_replay {
// One owned D3D8 device in the owned process. Preserve vtable identity and wrap
// only the three SDK method slots; forward every call and HRESULT unchanged.
class RenderTiming {
    using Scene = HRESULT(WINAPI *)(void *);
    using Present = HRESULT(WINAPI *)(void *, const RECT *, const RECT *, HWND, const RGNDATA *);
    inline static RenderTiming *instance_ = nullptr;
    Scene begin_ = nullptr, end_ = nullptr;
    Present present_ = nullptr;
    MappedJournal<th06::render_timing::Sample> journal_;
    void *device_ = nullptr;
    LONGLONG frequency_ = 0, anchor_ = 0, draw_begin_ = 0, draw_ticks_ = 0, previous_end_ = 0;
    std::uint64_t sequence_ = 0, stage_ = 0, draw_calls_ = 0, last_sequence_ = 0;
    bool failed_ = false;
    void (*before_present_)() = nullptr;
    static LONGLONG tick() {
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        return value.QuadPart;
    }
    std::uint64_t ns(LONGLONG ticks) const {
        return ticks > 0 ? th06::render_timing::nanoseconds(std::uint64_t(ticks),
                                                            std::uint64_t(frequency_))
                         : 0;
    }
    static HRESULT WINAPI begin_scene(void *device) {
        auto &self = *instance_;
        if (device != self.device_)
            return self.begin_(device);
        if (self.before_present_)
            self.before_present_();
        const auto start = tick();
        const auto result = self.begin_(device);
        if (SUCCEEDED(result))
            self.draw_begin_ = start;
        return result;
    }
    static HRESULT WINAPI end_scene(void *device) {
        auto &self = *instance_;
        if (device != self.device_)
            return self.end_(device);
        const auto result = self.end_(device);
        const auto finish = tick();
        if (self.draw_begin_) {
            self.draw_ticks_ += finish - self.draw_begin_;
            ++self.draw_calls_;
            self.draw_begin_ = 0;
        }
        return result;
    }
    static HRESULT WINAPI present(void *device, const RECT *source, const RECT *target, HWND window,
                                  const RGNDATA *dirty) {
        auto &self = *instance_;
        if (device != self.device_)
            return self.present_(device, source, target, window, dirty);
        if (self.before_present_)
            self.before_present_();
        const auto start = tick();
        const auto result = self.present_(device, source, target, window, dirty);
        const auto finish = tick();
        if (self.sequence_) {
            namespace t = th06::render_timing;
            t::Sample row{};
            row[t::Sequence] = self.sequence_;
            row[t::Stage] = self.stage_;
            row[t::ElapsedNs] = self.ns(finish - self.anchor_);
            row[t::PeriodNs] = self.previous_end_ ? self.ns(finish - self.previous_end_) : 0;
            row[t::DrawNs] = self.ns(self.draw_ticks_);
            row[t::PresentNs] = self.ns(finish - start);
            row[t::DrawCalls] = self.draw_calls_;
            row[t::Flags] = (!self.draw_calls_ ? t::NoDraw : 0) |
                            (FAILED(result) ? t::PresentFailed : 0) |
                            (self.sequence_ == self.last_sequence_ ? t::RepeatedReceipt : 0);
            self.failed_ = !self.journal_.append(row) || self.failed_;
            self.last_sequence_ = self.sequence_;
        }
        self.previous_end_ = finish;
        self.draw_ticks_ = 0;
        self.draw_calls_ = 0;
        return result;
    }

  public:
    std::uint64_t elapsed_ns(LONGLONG ticks) const {
        return ns(ticks);
    }
    void before_present(void (*callback)()) {
        before_present_ = callback;
    }
    bool open(const char *path, LONGLONG frequency, unsigned capacity) {
        if (frequency <= 0 || !th06::render_timing::frequency_valid(std::uint64_t(frequency)))
            return false;
        frequency_ = frequency;
        return journal_.open(path, th06::render_timing::header[0], capacity + 512);
    }
    bool install(void *device) {
        if (device_)
            return device_ == device;
        if (!device || !frequency_)
            return false;
        auto **original = *static_cast<void ***>(device);
        DWORD protection;
        if (!VirtualProtect(original + 15, 21 * sizeof(void *), PAGE_READWRITE, &protection))
            return false;
        present_ = reinterpret_cast<Present>(original[15]);
        begin_ = reinterpret_cast<Scene>(original[34]);
        end_ = reinterpret_cast<Scene>(original[35]);
        device_ = device;
        instance_ = this;
        InterlockedExchangePointer(original + 15, reinterpret_cast<void *>(present));
        InterlockedExchangePointer(original + 34, reinterpret_cast<void *>(begin_scene));
        InterlockedExchangePointer(original + 35, reinterpret_cast<void *>(end_scene));
        DWORD ignored;
        return VirtualProtect(original + 15, 21 * sizeof(void *), protection, &ignored) != 0;
    }
    void context(std::uint64_t sequence, unsigned stage, LONGLONG anchor) {
        sequence_ = sequence;
        stage_ = stage;
        anchor_ = anchor;
    }
    bool failed() const {
        return failed_;
    }
    bool close() {
        failed_ = !journal_.close() || failed_;
        return !failed_;
    }
};
} // namespace wine_replay
