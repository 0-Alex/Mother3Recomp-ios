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
