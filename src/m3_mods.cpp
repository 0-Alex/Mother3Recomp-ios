#include "m3_mods.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <utility>

#if defined(GBAGAME_RECOMP_UI)
#include "ips_patch.h"
#endif

namespace m3mods {
namespace {

namespace fs = std::filesystem;
constexpr std::size_t kCodeEnd = 0x140000;  // game.toml aot_scan_end - 0x08000000

struct Run { std::size_t at; std::vector<std::uint8_t> bytes; };
std::uint8_t* g_rom = nullptr;
std::vector<Run> g_deferred;

bool is_patch(const fs::path& p) {
    std::string ext = p.extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".ips" || ext == ".bps" || ext == ".ups";
}

std::vector<std::uint8_t> read(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

// Applies one patch to `rom` in place; false if it doesn't apply or changes the size.
bool patch_into(std::vector<std::uint8_t>& rom, const fs::path& file) {
#if defined(GBAGAME_RECOMP_UI)
    const auto patch = read(file);
    std::uint8_t* out = nullptr;
    std::size_t out_len = 0;
    if (patch.empty() || !rom_patch_apply(rom.data(), rom.size(), patch.data(), patch.size(), &out, &out_len))
        return false;
    const bool same_size = out_len == rom.size();
    if (same_size) std::memcpy(rom.data(), out, out_len);
    std::free(out);
    return same_size;
#else
    (void)rom; (void)file;
    return false;
#endif
}

std::set<std::string> enabled_names(const std::string& dir) {
    std::set<std::string> on;
    std::ifstream f(fs::path(folder(dir)) / "enabled.txt");
    for (std::string l; std::getline(f, l);) {
        while (!l.empty() && (l.back() == '\r' || l.back() == ' ')) l.pop_back();
        if (!l.empty() && l[0] != '#') on.insert(l);
    }
    return on;
}

bool code_differs(const std::uint8_t* a, const std::uint8_t* b) { return std::memcmp(a, b, kCodeEnd) != 0; }

}  // namespace

std::string folder(const std::string& dir) { return (fs::path(dir) / "mods").string(); }

std::vector<Mod> list(const std::string& dir) {
    std::vector<Mod> mods;
    std::error_code ec;
    fs::create_directories(folder(dir), ec);
    const auto on = enabled_names(dir);
    for (const auto& e : fs::directory_iterator(folder(dir), ec))
        if (e.is_regular_file() && is_patch(e.path())) {
            Mod m;
            m.file = e.path().filename().string();
            m.enabled = on.count(m.file) != 0;
            mods.push_back(m);
        }
    std::sort(mods.begin(), mods.end(), [](const Mod& a, const Mod& b) { return a.file < b.file; });
    return mods;
}

void save_enabled(const std::string& dir, const std::vector<Mod>& mods) {
    std::ofstream f(fs::path(folder(dir)) / "enabled.txt", std::ios::trunc);
    f << "# Mods ticked in the launcher, applied in name order.\n";
    for (const Mod& m : mods)
        if (m.enabled) f << m.file << "\n";
}

void classify(const std::string& dir, const std::vector<std::uint8_t>& rom, std::vector<Mod>& mods) {
    for (Mod& m : mods) {
        std::vector<std::uint8_t> copy = rom;
        m.valid = patch_into(copy, fs::path(folder(dir)) / m.file);
        m.changes_code = m.valid && code_differs(copy.data(), rom.data());
    }
}

bool apply(std::uint8_t* rom, std::size_t size, const std::string& dir) {
    g_deferred.clear();
    std::vector<Mod> mods = list(dir);
    if (std::none_of(mods.begin(), mods.end(), [](const Mod& m) { return m.enabled; })) return false;
    std::vector<std::uint8_t> out(rom, rom + size);
    for (const Mod& m : mods)
        if (m.enabled && !patch_into(out, fs::path(folder(dir)) / m.file))
            std::fprintf(stderr, "[mother3:mods] %s does not apply to this ROM; skipped\n", m.file.c_str());
    if (code_differs(out.data(), rom)) {
        std::memcpy(rom, out.data(), size);
        std::fprintf(stderr, "[mother3:mods] a mod changes game code; using the compatibility interpreter\n");
        return true;
    }
    for (std::size_t i = 0; i < size;) {  // remember data-only changes as runs
        if (out[i] == rom[i]) { ++i; continue; }
        std::size_t j = i;
        while (j < size && out[j] != rom[j]) ++j;
        g_deferred.push_back({i, std::vector<std::uint8_t>(out.begin() + i, out.begin() + j)});
        i = j;
    }
    g_rom = rom;
    return false;
}

void apply_deferred() {
    if (!g_rom) return;
    for (const Run& r : g_deferred) std::memcpy(g_rom + r.at, r.bytes.data(), r.bytes.size());
    if (!g_deferred.empty())
        std::fprintf(stderr, "[mother3:mods] applied %zu data-only change runs\n", g_deferred.size());
    g_deferred.clear();
    g_rom = nullptr;
}

}  // namespace m3mods
