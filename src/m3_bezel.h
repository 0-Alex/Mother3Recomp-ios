// In-game bezel composed from the player's ROM (Tazmily map, window frame,
// party sprites) instead of shipped artwork. mother3_bezel.dll draws it.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace m3bezel {

// Game area inside the 1920x1080 bezel: 3:2 at 6x. mother3_bezel.c uses the same numbers.
constexpr int kGameX = 240, kGameY = 60, kGameW = 1440, kGameH = 960;

// Writes <dir>/bezel_cache/m3_bezel_1080p.raw (RGBA8888, game area
// transparent) unless an up-to-date copy for this flavor already exists.
void ensure(const std::uint8_t* rom, std::size_t size, const std::string& dir, int flavor);

}  // namespace m3bezel
