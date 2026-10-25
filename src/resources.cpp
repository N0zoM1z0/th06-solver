#include "th06/resources.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>

namespace th06::solver {
namespace {
constexpr std::size_t max_resource = 64 * 1024 * 1024;
const Resources *active_resources = nullptr;
[[noreturn]] void corrupt(const std::string &reason) {
    throw std::runtime_error("PBG3: " + reason);
}
std::string canonical(std::string value) {
    for (auto &c : value) {
        if (c == '\\')
            c = '/';
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
    }
    return value;
}
std::vector<std::uint8_t> load(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        throw std::runtime_error("cannot open " + path.string());
    auto size = in.tellg();
    if (size < 0 || size > std::streamoff(max_resource))
        corrupt("archive too large");
    std::vector<std::uint8_t> result(static_cast<std::size_t>(size));
    in.seekg(0);
    if (!in.read(reinterpret_cast<char *>(result.data()), size))
        corrupt("short read");
    return result;
}
class Bits {
  public:
    Bits(const std::vector<std::uint8_t> &bytes, std::size_t begin, std::size_t end)
        : bytes_(bytes), bit_(begin * 8), end_(end * 8) {
        if (begin > end || end > bytes.size())
            corrupt("invalid bit range");
    }
    std::uint32_t read(unsigned count) {
        if (count > 32 || count > end_ - bit_)
            corrupt("truncated bit stream");
        std::uint32_t value = 0;
        while (count--) {
            value = (value << 1) | ((bytes_[bit_ / 8] >> (7 - bit_ % 8)) & 1);
            ++bit_;
        }
        return value;
    }
    std::uint32_t varint() {
        const auto width = (read(2) + 1) * 8;
        return read(width);
    }
    std::size_t consumed_bytes() const {
        return (bit_ + 7) / 8;
    }

  private:
    const std::vector<std::uint8_t> &bytes_;
    std::size_t bit_, end_;
};
} // namespace

Archive::Archive(const std::filesystem::path &path) : bytes_(load(path)) {
    parse();
}
Archive::Archive(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {
    parse();
}
std::uint64_t Archive::fingerprint() const {
    std::uint64_t hash = 14695981039346656037ULL;
    for (auto value : bytes_) {
        hash ^= value;
        hash *= 1099511628211ULL;
    }
    return hash;
}
void Archive::parse() {
    if (bytes_.size() < 6 || !std::equal(bytes_.begin(), bytes_.begin() + 4, "PBG3"))
        corrupt("bad signature");
    Bits header(bytes_, 4, bytes_.size());
    const auto count = header.varint(), table = header.varint();
    if (!count || count > 65536 || table < header.consumed_bytes() || table >= bytes_.size())
        corrupt("invalid table");
    Bits index(bytes_, table, bytes_.size());
    for (std::uint32_t i = 0; i < count; ++i) {
        index.varint();
        index.varint();
        ArchiveEntry e{};
        e.checksum = index.varint();
        e.offset = index.varint();
        e.size = index.varint();
        bool terminated = false;
        for (unsigned j = 0; j < 256; ++j) {
            const char c = static_cast<char>(index.read(8));
            if (!c) {
                terminated = true;
                break;
            }
            e.name += c;
        }
        if (!terminated || e.name.empty() || e.size > max_resource ||
            e.offset < header.consumed_bytes() || e.offset >= table ||
            (!entries_.empty() && e.offset <= entries_.back().offset))
            corrupt("invalid entry");
        if (!entries_.empty())
            entries_.back().compressed_size = e.offset - entries_.back().offset;
        entries_.push_back(std::move(e));
    }
    entries_.back().compressed_size = table - entries_.back().offset;
}

std::vector<std::uint8_t> Archive::extract(const ArchiveEntry &entry) const {
    const std::size_t begin = entry.offset, size = entry.compressed_size;
    if (entry.size > max_resource || begin > bytes_.size() || size > bytes_.size() - begin)
        corrupt("entry outside archive");
    Bits in(bytes_, begin, begin + size);
    std::array<std::uint8_t, 8192> dict{};
    std::size_t head = 1;
    std::vector<std::uint8_t> out;
    out.reserve(entry.size);
    auto emit = [&](std::uint8_t value) {
        if (out.size() >= entry.size)
            corrupt("decoded length overflow: " + entry.name);
        out.push_back(value);
        dict[head] = value;
        head = (head + 1) & 8191;
    };
    for (;;) {
        if (in.read(1))
            emit(static_cast<std::uint8_t>(in.read(8)));
        else {
            const auto offset = in.read(13);
            if (!offset)
                break;
            const auto length = in.read(4) + 3;
            for (unsigned j = 0; j < length; ++j)
                emit(dict[(offset + j) & 8191]);
        }
    }
    std::uint32_t checksum = 0;
    for (std::size_t i = begin; i < in.consumed_bytes(); ++i)
        checksum += bytes_[i];
    if (out.size() != entry.size || checksum != entry.checksum)
        corrupt("length/checksum mismatch: " + entry.name);
    return out;
}

Resources::Resources(const std::filesystem::path &directory) {
    std::vector<std::filesystem::path> paths;
    for (const auto &file : std::filesystem::directory_iterator(directory))
        if (file.is_regular_file() && canonical(file.path().extension().string()) == ".dat")
            paths.push_back(file.path());
    std::sort(paths.begin(), paths.end());
    if (paths.empty())
        throw std::runtime_error("no DAT archives in " + directory.string());
    for (const auto &path : paths) {
        const auto a = archives_.size();
        archives_.emplace_back(path);
        const auto &entries = archives_.back().entries();
        for (std::size_t e = 0; e < entries.size(); ++e) {
            const auto [found, added] =
                index_.emplace(canonical(entries[e].name), std::make_pair(a, e));
            if (!added) {
                const auto [prior_a, prior_e] = found->second;
                if (archives_[prior_a].extract(archives_[prior_a].entries()[prior_e]) !=
                    archives_[a].extract(entries[e]))
                    throw std::runtime_error("conflicting resource: " + entries[e].name);
            }
        }
    }
}
std::vector<std::uint8_t> Resources::read(const std::string &name) const {
    const auto normalized = canonical(name);
    const auto slash = normalized.find_last_of('/');
    const auto found =
        index_.find(slash == std::string::npos ? normalized : normalized.substr(slash + 1));
    if (found == index_.end())
        throw std::runtime_error("missing resource: " + name);
    const auto [a, e] = found->second;
    return archives_[a].extract(archives_[a].entries()[e]);
}
void Resources::verify_all() const {
    for (const auto &archive : archives_)
        for (const auto &entry : archive.entries())
            archive.extract(entry);
}
std::uint64_t Resources::fingerprint() const {
    std::vector<std::uint64_t> values;
    for (const auto &archive : archives_)
        values.push_back(archive.fingerprint());
    std::sort(values.begin(), values.end());
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto value : values)
        for (unsigned i = 0; i < 8; ++i) {
            hash ^= (value >> (i * 8)) & 255;
            hash *= 1099511628211ULL;
        }
    return hash;
}
void set_resources(const Resources *value) {
    active_resources = value;
}
const Resources &resources() {
    if (!active_resources)
        throw std::logic_error("resources not initialized");
    return *active_resources;
}
} // namespace th06::solver
