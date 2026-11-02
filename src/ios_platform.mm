#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#include "host_platform.h"
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
// Metal renderer just presents drawables, so the session holds an idle
// CADisplayLink requesting the rate it wants (Info.plist also carries
// CADisableMinimumFrameDurationOnPhone, without which 120 Hz is capped). The
// link's timestamps also give FramePacer the display's refresh grid to lock
// onto (gbarecomp::set_display_refresh_source), and with vsync its ticks are
// what the pacer waits on (gbarecomp::set_display_vsync_source).
//
// The link runs on its own thread: the game loop owns the main thread and
// only spins its run loop while pumping events, so a main-thread link fired
// late and in bursts, never on the refresh itself.
namespace {
std::mutex g_link_mutex;
std::condition_variable g_link_cv;
uint64_t g_refresh_count = 0;
std::chrono::steady_clock::time_point g_last_refresh;
std::chrono::nanoseconds g_refresh_interval{};

bool display_refresh(std::chrono::steady_clock::time_point* last, std::chrono::nanoseconds* interval) {
    std::lock_guard<std::mutex> lock(g_link_mutex);
    if (g_refresh_interval.count() <= 0) return false;
    *last = g_last_refresh;
    *interval = g_refresh_interval;
    return true;
}

uint64_t vsync_count() {
    std::lock_guard<std::mutex> lock(g_link_mutex);
    return g_refresh_count;
}

uint64_t vsync_wait(uint64_t target, std::chrono::nanoseconds timeout) {
    std::unique_lock<std::mutex> lock(g_link_mutex);
    g_link_cv.wait_for(lock, timeout, [&] { return g_refresh_count >= target; });
    return g_refresh_count;
}

const gbarecomp::DisplayVsyncSource kVsyncSource{&vsync_count, &vsync_wait};
}  // namespace

@interface M3RefreshRequest : NSObject
- (void)tick:(CADisplayLink*)link;
@end
@implementation M3RefreshRequest
- (void)tick:(CADisplayLink*)link {
    // CADisplayLink times are CACurrentMediaTime(); carry them over to
    // steady_clock through the time elapsed since the refresh.
    const auto now = std::chrono::steady_clock::now();
    const double age = CACurrentMediaTime() - link.timestamp;
    const auto last = now - std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::duration<double>(age));
    const double interval = link.targetTimestamp - link.timestamp;
    {
        std::lock_guard<std::mutex> lock(g_link_mutex);
        g_last_refresh = last;
        if (interval > 0.002 && interval < 0.05)
            g_refresh_interval = std::chrono::nanoseconds(static_cast<long long>(interval * 1e9 + 0.5));
        ++g_refresh_count;
    }
    g_link_cv.notify_all();
}
@end

void m3_ios_start_display_link(bool high_refresh, bool vsync) {
    static NSThread* thread = nil;
    if (thread) return;
    // A fixed rate, not a range: a ProMotion panel left free to settle on 80 or
    // 90 Hz can't show 60 Hz frames (or 120 Hz half frames) evenly.
    const float max_hz = static_cast<float>([UIScreen mainScreen].maximumFramesPerSecond);
    const float hz = high_refresh ? max_hz : std::min(60.0f, max_hz);
    thread = [[NSThread alloc] initWithBlock:^{
        static M3RefreshRequest* target = [M3RefreshRequest new];
        CADisplayLink* link = [CADisplayLink displayLinkWithTarget:target selector:@selector(tick:)];
        link.preferredFrameRateRange = CAFrameRateRangeMake(hz, hz, hz);
        [link addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        for (;;) {
            @autoreleasepool {
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
            }
        }
    }];
    thread.name = @"m3-display-link";
    thread.qualityOfService = NSQualityOfServiceUserInteractive;  // wake on the refresh, not after it
    [thread start];
    gbarecomp::set_display_refresh_source(&display_refresh);
    if (vsync) gbarecomp::set_display_vsync_source(&kVsyncSource);
    std::fprintf(stderr, "ios: requested %.0f Hz presentation, vsync %s\n", hz, vsync ? "on" : "off");
}

void m3_ios_configure_audio_session() {
    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        NSError* err = nil;
        const BOOL ok = [session setPreferredIOBufferDuration:0.005 error:&err];
        std::fprintf(stderr, "ios: preferred audio I/O buffer 5 ms %s (current %.1f ms)\n",
                     ok ? "requested" : "refused", session.IOBufferDuration * 1000.0);
    }
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
