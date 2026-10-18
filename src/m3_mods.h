// Mods: IPS / IPS32 / BPS / UPS patch files in <exe dir>/mods, applied in
// name order on top of the English v1.3 ROM when ticked in the launcher.
//
// The recompiled code covers ROM 0x000000-0x140000. A mod that leaves that
// range alone is a data mod (text, graphics, music) and is applied after the
// runtime hashes the ROM, so the game stays on its native code path. A mod
// that changes code is applied up front and runs on the compatibility
// interpreter.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace m3mods {

struct Mod {
    std::string file;      // file name inside mods/
    bool enabled = false;
    bool changes_code = false;  // filled by classify()
    bool valid = true;          // patch applied cleanly to this ROM
};

std::string folder(const std::string& dir);
std::vector<Mod> list(const std::string& dir);
void save_enabled(const std::string& dir, const std::vector<Mod>& mods);
// Tries each mod against `rom` to fill changes_code / valid.
void classify(const std::string& dir, const std::vector<std::uint8_t>& rom, std::vector<Mod>& mods);

// From rom_post_load_hook, on the stock image. Returns true when it changed
// code bytes now (the caller must re-hash); data-only edits wait for apply_deferred().
bool apply(std::uint8_t* rom, std::size_t size, const std::string& dir);
// From the game thread once the ROM has been hashed.
void apply_deferred();

}  // namespace m3mods
