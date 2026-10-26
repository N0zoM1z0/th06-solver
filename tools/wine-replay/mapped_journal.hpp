#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <windows.h>

namespace wine_replay {
// One writer, append-only records. Publish the count after copying a record;
// readers never inspect the uncommitted preallocated tail. No per-frame syscall.
template <class Record> class MappedJournal {
    static_assert(std::is_trivially_copyable<Record>::value);
    HANDLE file_ = INVALID_HANDLE_VALUE, mapping_ = nullptr;
    unsigned char *data_ = nullptr;
    std::uint32_t capacity_ = 0, count_ = 0;

  public:
    std::uint32_t count() const {
        return count_;
    }
    bool open(const char *path, std::uint32_t magic, std::uint32_t capacity) {
        if (file_ != INVALID_HANDLE_VALUE || !capacity || capacity > 1000512)
            return false;
        capacity_ = capacity;
        file_ = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE)
            return false;
        mapping_ = CreateFileMappingA(file_, nullptr, PAGE_READWRITE, 0,
                                      DWORD(16 + std::size_t(capacity) * sizeof(Record)), nullptr);
        if (!mapping_)
            return false;
        data_ = static_cast<unsigned char *>(
            MapViewOfFile(mapping_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0));
        if (!data_)
            return false;
        const std::uint32_t header[]{0, 2, sizeof(Record), 0};
        std::memcpy(data_, header, sizeof(header));
        InterlockedExchange(reinterpret_cast<volatile LONG *>(data_), LONG(magic));
        return true;
    }
    bool append(const Record &record) {
        if (!data_ || count_ >= capacity_)
            return false;
        std::memcpy(data_ + 16 + std::size_t(count_) * sizeof(Record), &record, sizeof(record));
        ++count_;
        InterlockedExchange(reinterpret_cast<volatile LONG *>(data_ + 12), LONG(count_));
        return true;
    }
    bool close() {
        bool good = true;
        if (data_) {
            good = FlushViewOfFile(data_, 0) != 0;
            good = (UnmapViewOfFile(data_) != 0) && good;
            data_ = nullptr;
        }
        if (mapping_) {
            good = (CloseHandle(mapping_) != 0) && good;
            mapping_ = nullptr;
        }
        if (file_ != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER length{};
            length.QuadPart = 16 + std::uint64_t(count_) * sizeof(Record);
            good = (SetFilePointerEx(file_, length, nullptr, FILE_BEGIN) != 0) && good;
            good = (SetEndOfFile(file_) != 0) && good;
            good = (FlushFileBuffers(file_) != 0) && good;
            good = (CloseHandle(file_) != 0) && good;
            file_ = INVALID_HANDLE_VALUE;
        }
        return good;
    }
};
} // namespace wine_replay
