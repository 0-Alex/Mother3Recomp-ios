#pragma once
#include <string>
#include <vector>

// iOS counterpart of gbarecomp::mobile_prepare_process (Android): makes the
// app's Documents folder the working directory, creates bios/ roms/ saves/
// mods/ there (visible in the Files app), installs the bundled game config,
// redirects diagnostics to ios-runtime.log and disables on-device self-heal.
bool m3_ios_prepare_process(std::vector<std::string>& args);

// iOS counterpart of gbarecomp::mobile_run_with_stack. SDL must keep running
// on the UIKit main thread (~1 MiB stack), so instead of a new thread this
// switches the main thread onto a reserved 256 MiB stack for the call.
int m3_ios_run_with_stack(int (*entry)(int, char**), int argc, char** argv);

// Hold the display at a fixed refresh rate (ProMotion's full rate for smooth
// motion, which needs 120 Hz; otherwise 60 Hz) and hand its refresh grid to
// FramePacer so frames are paced in step with the display. With vsync the
// pacer also waits on the link's refresh ticks (its own thread) instead of
// a timer.
void m3_ios_start_display_link(bool high_refresh, bool vsync);

// Ask for a short hardware I/O buffer (~5 ms instead of iOS's ~20 ms
// default) before SDL opens audio: less delay between the game's music and
// the speaker, which is what rhythm combos are timed against.
void m3_ios_configure_audio_session();
