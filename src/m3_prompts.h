// In-game button prompts: the script's "A Button" / "Start" mentions are drawn
// with the button the player actually uses (Xbox, PlayStation, Nintendo or
// keyboard), switching live when the controller in use changes.
#pragma once
#include <cstddef>
#include <cstdint>

namespace m3prompts {

// Remembers the loaded ROM image. Changes nothing yet, so the ROM still hashes
// as the stock image and keeps the native code path.
void attach(std::uint8_t* rom, std::size_t size, const char* source_sha1);

// Call regularly from the game thread; it rate-limits itself. Rewrites the
// script once, then redraws the button glyphs when the device or binds change.
void tick();

}  // namespace m3prompts
