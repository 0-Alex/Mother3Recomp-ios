// Game-owned MOTHER 3 launcher drawn entirely from the player's ROM.
#pragma once
#include <string>

#include "m3_assets.h"
#include "m3_settings.h"

namespace m3launcher {

enum class Result { Play, Quit };

// Opens its own SDL window and returns once the player picks Play or quits.
// Edits `settings` in place and saves them (and gamepad binds) under `dir`.
Result run(const m3assets::Rom& rom, m3settings::Settings& settings, const std::string& dir);

}  // namespace m3launcher
