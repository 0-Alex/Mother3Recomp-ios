#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
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
    // host_window only turns the on-screen pad on by itself under __ANDROID__;
    // iOS reaches it through the desktop env switch.
    setenv("GBARECOMP_TOUCH_CONTROLS", "1", 0);
    // SDL's reported DPI is unreliable on iOS and pad sizes are in mm. The
    // high-DPI drawable is in native pixels: ~160 points/inch times the
    // screen's native scale (3 on current iPhones).
    char dpi[32];
    std::snprintf(dpi, sizeof dpi, "%.0f", 160.0 * [UIScreen mainScreen].nativeScale);
    setenv("GBARECOMP_TOUCH_DPI", dpi, 0);
    std::fprintf(stderr, "ios: working directory is the app Documents folder\n");
    if (args.empty()) args.emplace_back("main");
    args[0] = "./Mother3Recomp";
    args.emplace_back("variants/mother3en/game.toml");
   
    return true;
}

// ProMotion only runs an app above 60 Hz while something asks for it: SDL's
// Metal renderer just presents drawables, so a smooth-motion session holds an
// idle CADisplayLink requesting 120 Hz (Info.plist also carries
// CADisableMinimumFrameDurationOnPhone, without which the request is capped).
@interface M3RefreshRequest : NSObject
- (void)tick:(CADisplayLink*)link;
@end
@implementation M3RefreshRequest
- (void)tick:(CADisplayLink*)link {}
@end

void m3_ios_request_high_refresh() {
    static CADisplayLink* link = nil;
    if (link) return;
    static M3RefreshRequest* target = [M3RefreshRequest new];
    link = [CADisplayLink displayLinkWithTarget:target selector:@selector(tick:)];
    const float max_hz = static_cast<float>([UIScreen mainScreen].maximumFramesPerSecond);
    link.preferredFrameRateRange = CAFrameRateRangeMake(std::min(80.0f, max_hz), max_hz, max_hz);
    [link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    std::fprintf(stderr, "ios: requested %.0f Hz presentation\n", max_hz);
}

// m3_call_on_stack(fn, arg, stack_top): call fn(arg) with sp = stack_top,
// then return on the original stack (arm64 only; iOS devices are arm64).
extern "C" void m3_call_on_stack(void (*fn)(void*), void* arg, void* stack_top);
__asm__(
    ".text\n"
    ".globl _m3_call_on_stack\n"
    ".p2align 2\n"
    "_m3_call_on_stack:\n"
    "    stp x29, x30, [sp, #-16]!\n"
    "    mov x29, sp\n"
    "    mov sp, x2\n"
    "    mov x9, x0\n"
    "    mov x0, x1\n"
    "    blr x9\n"
    "    mov sp, x29\n"
    "    ldp x29, x30, [sp], #16\n"
    "    ret\n");

namespace {
struct StackedCall {
    int (*entry)(int, char**);
    int argc;
    char** argv;
    int result;
};
void run_stacked(void* arg) {
    auto* call = static_cast<StackedCall*>(arg);
    call->result = call->entry(call->argc, call->argv);
}
}  // namespace

int m3_ios_run_with_stack(int (*entry)(int, char**), int argc, char** argv) {
    // Generated guest calls nest deeply (desktop and Android reserve 256 MiB).
    constexpr size_t kStackBytes = 256u * 1024u * 1024u;
    constexpr size_t kGuardBytes = 64u * 1024u;
    void* base = mmap(nullptr, kStackBytes + kGuardBytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);
    if (base == MAP_FAILED) {
        std::fprintf(stderr, "ios: could not reserve the game stack; running on the main thread stack\n");
        return entry(argc, argv);
    }
    mprotect(base, kGuardBytes, PROT_NONE);  // overflow faults instead of corrupting the heap
    std::fprintf(stderr, "ios: game stack=%zu bytes\n", kStackBytes);
    StackedCall call{entry, argc, argv, 1};
    m3_call_on_stack(run_stacked, &call, static_cast<char*>(base) + kGuardBytes + kStackBytes);
    munmap(base, kStackBytes + kGuardBytes);
    return call.result;
}

// Entry point. SDL2main is not linked, so hand SDL_main to SDL's UIKit app
// delegate here (this is what SDL2main's main() does on iOS).
extern "C" int SDL_UIKitRunApp(int argc, char* argv[], int (*mainFunction)(int, char*[]));
extern "C" int SDL_main(int argc, char** argv);

int main(int argc, char** argv) {
    return SDL_UIKitRunApp(argc, argv, SDL_main);
}
