// Game-owned MOTHER 3 launcher drawn entirely from the player's ROM.
#pragma once
#include "m3_assets.h"

namespace m3launcher {

enum class Result { Play, Quit };

// Opens its own SDL window and returns once the player picks Play or quits.
Result run(const m3assets::Rom& rom);

}  // namespace m3launcher
