#include "test_support.hpp"
#include "th06/resources.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace th06::solver;
namespace {
struct Writer {
    std::vector<std::uint8_t> bytes;
    unsigned used = 0;
    void bits(std::uint32_t value, unsigned count) {
        for (unsigned i = count; i > 0; --i) {
            if (!used)
                bytes.push_back(0);
            bytes.back() |= ((value >> (i - 1)) & 1) << (7 - used);
            used = (used + 1) % 8;
        }
    }
    void var(std::uint32_t value) {
        unsigned width = value > 65535 ? 24 : value > 255 ? 16 : 8;
        bits(width / 8 - 1, 2);
        bits(value, width);
    }
};
std::vector<std::uint8_t> fixture(char first = 'A', unsigned decoded_size = 8) {
    Writer compressed;
    compressed.bits(1, 1);
    compressed.bits(first, 8);
    compressed.bits(1, 1);
    compressed.bits('B', 8);
    compressed.bits(0, 1);
    compressed.bits(1, 13);
    compressed.bits(3, 4);
    compressed.bits(0, 1);
    compressed.bits(0, 13);
    unsigned sum = 0;
    for (auto b : compressed.bytes)
        sum += b;
    Writer header;
    header.var(1);
    header.var(10 + compressed.bytes.size());
    std::vector<std::uint8_t> bytes{'P', 'B', 'G', '3'};
    bytes.insert(bytes.end(), header.bytes.begin(), header.bytes.end());
    bytes.resize(10);
    bytes.insert(bytes.end(), compressed.bytes.begin(), compressed.bytes.end());
    Writer table;
    table.var(0);
    table.var(0);
    table.var(sum);
    table.var(10);
    table.var(decoded_size);
    for (char c : std::string("fixture.bin"))
        table.bits(c, 8);
    table.bits(0, 8);
    bytes.insert(bytes.end(), table.bytes.begin(), table.bytes.end());
    return bytes;
}
void write(const std::filesystem::path &path, const std::vector<std::uint8_t> &data) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(data.data()), data.size());
    check(bool(out), "fixture write failed");
}
} // namespace
int main(int argc, char **argv) {
    try {
        const auto valid = fixture();
        Archive archive(valid);
        const auto decoded = archive.extract(archive.entries().at(0));
        check(std::string(decoded.begin(), decoded.end()) == "ABABABAB",
              "overlapping dictionary backreference");
        for (std::size_t n = 0; n < valid.size(); ++n)
            rejects(
                [&] {
                    Archive truncated(std::vector<std::uint8_t>(valid.begin(), valid.begin() + n));
                    truncated.extract(truncated.entries().at(0));
                },
                "truncated archive accepted");
        auto corrupt = valid;
        corrupt[10] ^= 1;
        rejects(
            [&] {
                Archive a(corrupt);
                a.extract(a.entries().at(0));
            },
            "bad compressed checksum accepted");
        for (unsigned size : {7u, 9u})
            rejects(
                [&] {
                    Archive a(fixture('A', size));
                    a.extract(a.entries().at(0));
                },
                "decoded length mismatch accepted");
        check(argc == 2, "test needs work directory");
        const std::filesystem::path work = argv[1];
        std::filesystem::create_directories(work);
        write(work / "one.DAT", valid);
        write(work / "two.dat", valid);
        Resources resources(work);
        resources.verify_all();
        check(resources.archive_count() == 2 && resources.entry_count() == 1,
              "identical shared resources");
        check(resources.read("DATA\\FIXTURE.BIN") == decoded, "native basename/case handling");
        rejects([&] { resources.read("missing.bin"); }, "missing resource accepted");
        write(work / "two.dat", fixture('C'));
        rejects([&] { Resources conflicts(work); }, "conflicting resources accepted");
        std::filesystem::remove(work / "one.DAT");
        std::filesystem::remove(work / "two.dat");
        std::cout << "PBG3 bounds, checksum, overlap and lookup contracts passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
