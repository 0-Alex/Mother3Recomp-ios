// Smooth motion (120 fps): on a display of 100 Hz or more, every real frame is
// preceded by an in-between one. It is the newest frame with each pixel moved
// back by half of its layer's or sprite's motion since the previous frame,
// drawn at twice the resolution so half-pixel steps show.
#pragma once

namespace m3smooth {

// 0 off, 1 on displays of 100 Hz or more, 2 always (for displays that misreport).
void enable(int level);  // installs the engine hooks; call before run_game()

}  // namespace m3smooth
