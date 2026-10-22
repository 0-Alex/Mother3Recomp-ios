#pragma once
#include <string>
#include <vector>

// iOS counterpart of gbarecomp::mobile_prepare_process (Android): makes the
// app's Documents folder the working directory, creates bios/ roms/ saves/
// mods/ there (visible in the Files app), installs the bundled game config,
// redirects diagnostics to ios-runtime.log and disables on-device self-heal.
bool m3_ios_prepare_process(std::vector<std::string>& args);
