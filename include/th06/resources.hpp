#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace th06::solver {
struct ArchiveEntry {
    std::string name;
    std::uint32_t checksum, offset, size, compressed_size;
};

// Owns the original compressed bytes. Each extraction has a fresh zeroed
// dictionary, as in Pbg3Archive::ReadDecompressEntry (TH06 1.02h).
class Archive {
  public:
    explicit Archive(const std::filesystem::path &path);
    explicit Archive(std::vector<std::uint8_t> bytes);
    const std::vector<ArchiveEntry> &entries() const {
        return entries_;
    }
    std::vector<std::uint8_t> extract(const ArchiveEntry &entry) const;
    std::uint64_t fingerprint() const;

  private:
    void parse();
    std::vector<std::uint8_t> bytes_;
    std::vector<ArchiveEntry> entries_;
};

class Resources {
  public:
    explicit Resources(const std::filesystem::path &directory);
    std::vector<std::uint8_t> read(const std::string &name) const;
    std::size_t archive_count() const {
        return archives_.size();
    }
    std::size_t entry_count() const {
        return index_.size();
    }
    void verify_all() const;
    std::uint64_t fingerprint() const;

  private:
    std::vector<Archive> archives_;
    std::map<std::string, std::pair<std::size_t, std::size_t>> index_;
};
void set_resources(const Resources *resources);
const Resources &resources();
} // namespace th06::solver
