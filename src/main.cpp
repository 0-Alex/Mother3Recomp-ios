// Mother3Recomp — native host for the statically recompiled
// MOTHER 3 (A3UJ) + MOTHER 3 Fan Translation v1.3 (English).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "runtime.h"
#include "runtime_arm.h"

#if defined(GBAGAME_RECOMP_UI)
#include "game_launcher_boot.h"
#endif

namespace {

constexpr const char* kEnglishSha1 = "306fb8874533b0fd0796208faa8bed2db4ae77fb";
constexpr const char* kJapanSha1   = "4f0f493e12c2a8c61b2d809af03f7abf87a85776";

struct RamTemplate { uint32_t rom; uint32_t size; };
// FLASH512_V131 helpers copied to RAM before each call (see game.toml).
constexpr RamTemplate kFlashTemplates[] = {
    {0x080912D8u, 0x30u},  // verify/compare loop
    {0x08091218u, 0x24u},  // byte copy loop
    {0x080911D4u, 0x04u},  // ldrb/bx lr read stub
};

bool ram_matches_rom(uint32_t ram, uint32_t rom, uint32_t size) {
    for (uint32_t off = 0; off < size; ++off)
        if (bus_read_u8(ram + off) != bus_read_u8(rom + off)) return false;
    return true;
}

bool in_ram(uint32_t a, uint32_t size) {
    const bool iw = a >= 0x03000000u && a + size <= 0x03008000u;
    const bool ew = a >= 0x02000000u && a + size <= 0x02040000u;
    return iw || ew;
}

// The library places these routines at varying stack depths (and reuses
// overlapping slots), so address-only dispatch cannot identify them. Match
// the complete live bytes against the hash-gated ROM template and run the
// template's AOT code at the equivalent offset. The routines contain no
// PC-relative data accesses, so executing them at the ROM address is exact.
int dispatch_flash_ram_variant(uint32_t pc, int thumb) {
    if (!thumb) return 0;
    for (const auto& t : kFlashTemplates) {
        uint32_t start = pc >= t.size - 2u ? pc - (t.size - 2u) : 0u;
        start = (start + 1u) & ~1u;
        for (; start <= pc; start += 2u) {
            if (!in_ram(start, t.size)) continue;
            if (!ram_matches_rom(start, t.rom, t.size)) continue;
            runtime_dispatch(t.rom + (pc - start));
            return 1;
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::printf(
                "Mother3Recomp [--bios <path>] [--rom <path>] [game.toml]\n\n"
                "Windowed play opens the launcher first; --no-launcher skips it once.\n"
                "Press Escape during play to open the in-game menu.\n");
            return 0;
        }
    }

    g_runtime_ram_dispatch_hook = &dispatch_flash_ram_variant;
    gbarecomp::RunOptions opts;
    opts.builtin_game_name = "MOTHER 3 (English Fan Translation v1.3)";
    opts.builtin_rom_sha1 = kEnglishSha1;
    opts.launcher_region = "Japan + English patch";
    opts.launcher_boxart = "assets/img/boxart_mother3.tga";  // original art, no Nintendo assets
    opts.launcher_game_config = "game.toml";
    opts.launcher_save_path = "saves/mother3_en_v13.sav";
    opts.launcher_expose_widescreen = true;
    opts.launcher_expose_adaptive_view = true;
    opts.widescreen_view_width = 284;
    // Clean Japanese cartridge + the fan translation patch → verified English image.
    opts.launcher_enable_rom_patches = true;
    opts.launcher_source_rom_sha1 = kJapanSha1;
    opts.launcher_required_patch_sha1 = kEnglishSha1;
    opts.launcher_accept_patched_rom = true;   // an existing English v1.3 ROM works too
    opts.launcher_rom_patch_note =
        "Using the clean MOTHER 3 (Japan) ROM? Choose the fan translation's "
        "mother3.ups (v1.3) here and it is applied automatically. Already have "
        "the English v1.3 ROM? Select it above and leave this off.";

    // PC conveniences: rewind + fast-forward, save states, sharper scaling.
    opts.expose_assist_tools = true;
    opts.assist_tools_enabled_by_default = true;
    opts.assist_fast_forward_multiplier_default = 4;
    opts.rewind_history_seconds = 30;
    opts.launcher_expose_sharp_filter = true;
    opts.launcher_default_sharp_filter = true;
    opts.freely_resizable_window = true;
    opts.max_view_width = 432;

#if defined(GBAGAME_RECOMP_UI)
    std::vector<std::string> args(argv, argv + argc);
    if (game_launcher_preboot(args, opts)) return 0;
    std::vector<char*> av;
    av.reserve(args.size());
    for (auto& arg : args) av.push_back(arg.data());
    return gbarecomp::run_game(static_cast<int>(av.size()), av.data(), opts);
#else
    return gbarecomp::run_game(argc, argv, opts);
#endif
}
