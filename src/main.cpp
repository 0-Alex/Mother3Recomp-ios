// Mother3Recomp â€” native host for the statically recompiled
// MOTHER 3 (A3UJ) + MOTHER 3 Fan Translation v1.3 (English) and
// High-Quality Sound Mixing Mod (m4a_hq_mixer).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "runtime.h"
#include "runtime_arm.h"
#include "mother3_runtime_hooks.h"
#include "m3_assets.h"
#include "m3_launcher.h"
#include "m3_settings.h"
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__ANDROID__)
#include <algorithm>
#include <limits>
#include <jni.h>
#include "ips_patch.h"
#include "mobile_platform.h"
#elif defined(__linux__)
#include <sys/resource.h>
#endif

#if defined(GBAGAME_RECOMP_UI)
#include "game_launcher_boot.h"
#endif

extern "C" const unsigned kDispatchTableLen;

namespace {
constexpr const char* kEnglishSha1 = "306fb8874533b0fd0796208faa8bed2db4ae77fb";
constexpr const char* kHqAudioSha1 = "4ffe71ce5c5e1e8d9be5fbd09968b60787169b21";
constexpr const char* kJapanSha1   = "4f0f493e12c2a8c61b2d809af03f7abf87a85776";

std::string arg_value(const std::vector<std::string>& args, const char* flag) {
    for (std::size_t i = 1; i + 1 < args.size(); ++i)
        if (args[i] == flag) return args[i + 1];
    return {};
}

// First line of a path file the classic launcher writes (rom.cfg / bios.cfg).
std::string cfg_path(const std::string& name) {
    std::FILE* f = std::fopen(name.c_str(), "rb");
    if (!f) return {};
    char line[1024] = {};
    if (!std::fgets(line, sizeof line, f)) line[0] = 0;
    std::fclose(f);
    std::string s = line;
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::vector<std::uint8_t> out;
    if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
        std::uint8_t buf[1 << 16];
        for (std::size_t n; (n = std::fread(buf, 1, sizeof buf, f)) > 0;) out.insert(out.end(), buf, buf + n);
        std::fclose(f);
    }
    return out;
}

// The game-owned launcher runs by default once a ROM is configured; until then
// (first run) the classic launcher handles BIOS/ROM setup. --classic-launcher
// forces the old one and --no-launcher skips both. Returns false when the
// player quits; on Play it skips the classic launcher.
bool m3_launcher_preboot(std::vector<std::string>& args) {
    bool requested = true;
    for (auto it = args.begin(); it != args.end();) {
        if (*it == "--classic-launcher") requested = false;
        if (*it == "--no-launcher") requested = false;
        if (*it == "--m3-launcher" || *it == "--classic-launcher") it = args.erase(it); else ++it;
    }
    if (!requested) return true;
    // Config files live next to the executable, where the runtime reads them.
    std::string dir = args[0];
    const auto slash = dir.find_last_of("/\\");
    dir = slash == std::string::npos ? "." : dir.substr(0, slash);
    std::string rom_path = arg_value(args, "--rom");
    if (rom_path.empty()) rom_path = cfg_path(dir + "/rom.cfg");
    m3assets::Rom rom(read_file(rom_path));
    if (!rom.ok()) {  // first run, or a ROM the new launcher can't draw from
        if (!rom_path.empty())
            std::fprintf(stderr, "m3-launcher: no usable English v1.3 ROM at '%s'; using the classic launcher.\n",
                         rom_path.c_str());
        return true;
    }
    m3settings::Settings settings = m3settings::load(dir);
    if (m3launcher::run(rom, settings, dir) == m3launcher::Result::Quit) return false;
    if (arg_value(args, "--rom").empty()) args.insert(args.end(), {"--rom", rom_path});
    const std::string bios = cfg_path(dir + "/bios.cfg");
    if (arg_value(args, "--bios").empty() && !bios.empty()) args.insert(args.end(), {"--bios", bios});
    // Right after argv[0], so flags typed on the command line still win.
    const auto flags = m3settings::game_args(settings);
    args.insert(args.begin() + 1, flags.begin(), flags.end());
    args.push_back("--no-launcher");
    return true;
}

}

int mother3_main(int argc, char** argv) {
#if defined(__linux__) && !defined(__ANDROID__)
    // Generated guest calls require the same stack budget as the Windows build.
    struct rlimit stack;
    constexpr rlim_t required_stack = 256u * 1024u * 1024u;
    if (getrlimit(RLIMIT_STACK, &stack) == 0 && stack.rlim_cur < required_stack) {
        stack.rlim_cur = required_stack;
        if (setrlimit(RLIMIT_STACK, &stack) != 0) {
            std::fprintf(stderr, "Could not reserve the game stack; raise the process stack limit.\n");
            return 1;
        }
    }
#endif
#if defined(_WIN32)
    char hook_path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, hook_path, MAX_PATH)) {
        if (char* slash = std::strrchr(hook_path, '\\')) {
            *slash = 0;
            std::string hook = std::string(hook_path) + "\\mother3_bezel.dll";
            if (GetFileAttributesA(hook.c_str()) != INVALID_FILE_ATTRIBUTES)
                _putenv_s("SDL_DYNAMIC_API", hook.c_str());
        }
    }
#endif
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::printf(
                "Mother3Recomp [--bios <path>] [--rom <path>] [game.toml]\n\n"
                "Windowed play opens the launcher first; --no-launcher skips it once and\n"
                "--classic-launcher opens the original setup launcher.\n"
                "F10 toggles the bezel, F11 opens the menu, Escape exits.\n");
            return 0;
        }
    }

    if (kDispatchTableLen == 0) {
#if defined(_WIN32)
        _putenv_s("GBARECOMP_FORCE_INTERP", "1");
#else
        setenv("GBARECOMP_FORCE_INTERP", "1", 1);
#endif
    }
    mother3_install_runtime_hooks();

    gbarecomp::RunOptions opts;
    opts.builtin_game_name = "MOTHER 3 (English Fan Translation v1.3)";
    opts.builtin_rom_sha1 = kEnglishSha1;
    opts.allow_rom_sha1_mismatch = true;
    opts.alt_rom_sha1 = kHqAudioSha1;
    opts.runtime_menu_scancode = 68;  // F11; Escape remains the exit key.
    opts.launcher_region = "Japan + fan translations";
    opts.launcher_boxart = "assets/img/boxart_mother3.tga";  // original art, no Nintendo assets
    opts.launcher_game_config = "game.toml";
    opts.launcher_save_path = "saves/mother3_en_v13.sav";
    opts.launcher_expose_widescreen = false;
    opts.launcher_expose_adaptive_view = false;
    opts.widescreen_view_width = 284;
    // Apply HQ audio in memory after the selected language patch is loaded.
    opts.launcher_enable_rom_patches = true;
    opts.launcher_source_rom_sha1 = kJapanSha1;
    opts.launcher_required_patch_sha1 = nullptr;
    opts.launcher_accept_patched_rom = true;   // supported patched language images work directly
    opts.rom_post_load_hook = &mother3_rom_post_load;
    opts.launcher_rom_patch_note =
        "HQ Audio is built in and applied to supported Japanese and fan-translated ROMs. "
        "IPS, IPS32, BPS or UPS language patches can be selected here.";

    // PC conveniences: rewind + fast-forward, save states, sharper scaling.
    opts.expose_assist_tools = true;
    opts.assist_tools_enabled_by_default = true;
    opts.assist_fast_forward_multiplier_default = 4;
    opts.rewind_history_seconds = 30;
    opts.launcher_expose_sharp_filter = true;
    opts.launcher_default_sharp_filter = true;
    opts.freely_resizable_window = true;
    opts.max_view_width = 284;  // ceiling only; --view-width (launcher setting) picks the width

    std::vector<std::string> args(argv, argv + argc);
#if defined(__ANDROID__)
    // App-private storage becomes the working directory, diagnostics go to
    // android-runtime.log and the APK payload's game.toml is appended.
    gbarecomp::MobileProcessOptions mobile;
    mobile.game_config = "variants/mother3en/game.toml";
    mobile.program_name = "./Mother3Recomp";
    gbarecomp::mobile_prepare_process(args, mobile);
    // The setup Activity is Android's launcher: it already ran, and it stores
    // the same config.ini [Launcher] keys the desktop launcher writes.
    args.erase(std::remove(args.begin(), args.end(), std::string("--no-launcher")), args.end());
    if (m3settings::load(".").widescreen) args.insert(args.begin() + 1, {"--view-width", "284"});
    opts.orientation = gbarecomp::RunOptions::Orientation::Landscape;
    opts.resume_suspend_state_on_launch = true;
    opts.ui_touch_friendly = true;
    opts.touch_pad_default = 1;
    // shortcut: rewind/fast-forward stay desktop-only until rewind memory is measured on phones.
    opts.expose_assist_tools = false;
    opts.assist_tools_enabled_by_default = false;
#else
    if (!m3_launcher_preboot(args)) return 0;
#if defined(GBAGAME_RECOMP_UI)
    if (game_launcher_preboot(args, opts)) return 0;
#endif
#endif
    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
}

#if defined(__ANDROID__)
// The setup Activity applies a language patch to the imported Japanese ROM.
extern "C" JNIEXPORT jbyteArray JNICALL
Java_org_gbarecomp_GbaNative_applyRomPatch(JNIEnv* env, jclass, jbyteArray source, jbyteArray patch) {
    if (!source || !patch) return nullptr;
    std::vector<std::uint8_t> source_bytes(static_cast<std::size_t>(env->GetArrayLength(source)));
    std::vector<std::uint8_t> patch_bytes(static_cast<std::size_t>(env->GetArrayLength(patch)));
    env->GetByteArrayRegion(source, 0, static_cast<jsize>(source_bytes.size()),
                            reinterpret_cast<jbyte*>(source_bytes.data()));
    env->GetByteArrayRegion(patch, 0, static_cast<jsize>(patch_bytes.size()),
                            reinterpret_cast<jbyte*>(patch_bytes.data()));
    if (env->ExceptionCheck()) return nullptr;
    std::uint8_t* output = nullptr;
    std::size_t output_size = 0;
    if (!rom_patch_apply(source_bytes.data(), source_bytes.size(), patch_bytes.data(), patch_bytes.size(),
                         &output, &output_size))
        return nullptr;
    if (output_size > static_cast<std::size_t>(std::numeric_limits<jsize>::max())) {
        std::free(output);
        return nullptr;
    }
    jbyteArray result = env->NewByteArray(static_cast<jsize>(output_size));
    if (result)
        env->SetByteArrayRegion(result, 0, static_cast<jsize>(output_size), reinterpret_cast<const jbyte*>(output));
    std::free(output);
    return env->ExceptionCheck() ? nullptr : result;
}

extern "C" int SDL_main(int argc, char** argv) {
    // The generated guest calls need a deeper stack than SDLActivity's thread.
    const int result = gbarecomp::mobile_run_with_stack(mother3_main, argc, argv);
    if (std::FILE* status = std::fopen("android-exit-code", "w")) {  // read by the setup Activity
        std::fprintf(status, "%d", result);
        std::fclose(status);
    }
    return result;
}
#else
int main(int argc, char** argv) { return mother3_main(argc, argv); }
#endif
