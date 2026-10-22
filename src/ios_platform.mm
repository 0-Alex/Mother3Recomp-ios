#import <Foundation/Foundation.h>

#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include "ios_platform.h"

bool m3_ios_prepare_process(std::vector<std::string>& args) {
    @autoreleasepool {
        NSArray<NSString*>* dirs = NSSearchPathForDirectoriesInDomains(
            NSDocumentDirectory, NSUserDomainMask, YES);
        if (dirs.count == 0) return false;
        NSString* docs = dirs[0];
        NSFileManager* fm = [NSFileManager defaultManager];
        for (NSString* sub in @[ @"bios", @"roms", @"saves", @"mods", @"variants/mother3en" ]) {
            [fm createDirectoryAtPath:[docs stringByAppendingPathComponent:sub]
          withIntermediateDirectories:YES
                           attributes:nil
                                error:nil];
        }
        // The CI packaging step copies the runtime config into the .app root.
        NSString* src = [[[NSBundle mainBundle] bundlePath]
            stringByAppendingPathComponent:@"game_ios_mother3en.toml"];
        NSString* dst = [docs stringByAppendingPathComponent:@"variants/mother3en/game.toml"];
        [fm removeItemAtPath:dst error:nil];
        [fm copyItemAtPath:src toPath:dst error:nil];
        if (chdir([docs fileSystemRepresentation]) != 0) return false;
    }
    std::freopen("ios-runtime.log", "w", stderr);
    std::fflush(stdout);
    dup2(fileno(stderr), fileno(stdout));
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    // No compiler on the device: dispatch misses must not try an on-device rebuild.
    setenv("GBARECOMP_SELFHEAL_RECOMPILE", "0", 0);
    std::fprintf(stderr, "ios: working directory is the app Documents folder\n");
    if (args.empty()) args.emplace_back("main");
    args[0] = "./Mother3Recomp";
    args.emplace_back("variants/mother3en/game.toml");
    args.emplace_back("--no-launcher");
    return true;
}

// Entry point. SDL2main is not linked, so hand SDL_main to SDL's UIKit app
// delegate here (this is what SDL2main's main() does on iOS).
extern "C" int SDL_UIKitRunApp(int argc, char* argv[], int (*mainFunction)(int, char*[]));
extern "C" int SDL_main(int argc, char** argv);

int main(int argc, char** argv) {
    return SDL_UIKitRunApp(argc, argv, SDL_main);
}
