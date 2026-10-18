#include "m3_prompts.h"

#include <SDL.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "m3_settings.h"
#if defined(GBAGAME_RECOMP_UI)
#include "gba_pad_binds.h"
#endif

namespace m3prompts {
namespace {

// English v1.3 (plain and with the HQ mixer applied) layout.
constexpr const char* kEnglishSha1 = "306fb8874533b0fd0796208faa8bed2db4ae77fb";
constexpr const char* kHqAudioSha1 = "4ffe71ce5c5e1e8d9be5fbd09968b60787169b21";
constexpr std::size_t kRomSize = 0x02000000;
constexpr std::size_t kFont = 0x00CE39F8;        // 16x16 1bpp glyphs, 4 tiles TL TR BL BR
constexpr std::size_t kWidths = 0x00D1CE78;      // per-glyph advance (pixel width + 1)
constexpr std::size_t kScript = 0x01370000, kScriptEnd = 0x01420000;

// Prompted GBA buttons and the Japanese-only glyph slots the English script never uses.
enum Button { kA, kB, kL, kR, kStart, kSelect, kCount };
constexpr std::uint8_t kSlot[kCount] = {0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA};
constexpr std::uint8_t kBlank = 0xDB;  // zero-width filler after a shortened word
// Index of each prompted button in m3settings / input.ini order.
constexpr int kBindIndex[kCount] = {4, 5, 6, 7, 8, 9};

enum Style { kKeyboard, kXbox, kPlayStation, kNintendo, kTouch };
#if defined(__ANDROID__)
constexpr Style kNoPad = kTouch;  // phones: the on-screen buttons carry GBA names
#else
constexpr Style kNoPad = kKeyboard;
#endif
const char* const kTouchLabels[kCount] = {"A", "B", "L", "R", "St", "Se"};

using Glyph = std::array<std::uint16_t, 8>;  // 8 rows; bit x = column x

// 8x8 icons for buttons whose names don't fit in a glyph.
struct Icon { const char* name; const char* rows[8]; };
const Icon kIcons[] = {
    {"cross", {"#......#", ".#....#.", "..#..#..", "...##...", "...##...", "..#..#..", ".#....#.", "#......#"}},
    {"circle", {"..####..", ".#....#.", "#......#", "#......#", "#......#", "#......#", ".#....#.", "..####.."}},
    {"square", {"#######.", "#.....#.", "#.....#.", "#.....#.", "#.....#.", "#.....#.", "#.....#.", "#######."}},
    {"triangle", {"...#....", "...#....", "..#.#...", "..#.#...", ".#...#..", ".#...#..", "#.....#.", "#######."}},
    {"menu", {"........", "#######.", "........", "#######.", "........", "#######.", "........", "........"}},
    {"view", {"..#####.", "..#...#.", "#####.#.", "#...#.#.", "#...###.", "#...#...", "#####...", "........"}},
    {"plus", {"...#....", "...#....", "...#....", "#######.", "...#....", "...#....", "...#....", "........"}},
    {"minus", {"........", "........", "........", "#######.", "........", "........", "........", "........"}},
    {"return", {"......#.", "......#.", "..#...#.", ".##...#.", "#######.", ".##.....", "..#.....", "........"}},
    {"shift", {"...#....", "..#.#...", ".#...#..", "###.###.", "..#.#...", "..#.#...", "..###...", "........"}},
    {"space", {"........", "........", "........", "........", "........", "#.....#.", "#######.", "........"}},
    {"up", {"...#....", "..###...", ".#.#.#..", "#..#..#.", "...#....", "...#....", "...#....", "........"}},
    {"down", {"...#....", "...#....", "...#....", "#..#..#.", ".#.#.#..", "..###...", "...#....", "........"}},
    {"left", {"...#....", "..#.....", ".#......", "#######.", ".#......", "..#.....", "...#....", "........"}},
    {"right", {"...#....", "....#...", ".....#..", "#######.", ".....#..", "....#...", "...#....", "........"}},
};

struct State {
    std::uint8_t* rom = nullptr;
    std::array<Glyph, 0x5F> letters{};  // stock ASCII glyphs, copied before any edit
    std::array<std::uint8_t, 0x5F> letter_width{};
    bool patched = false;
    Uint32 next_poll = 0, next_binds = 0;
    Style style = kKeyboard;
    std::array<std::string, kCount> drawn;  // what each slot currently shows
    m3settings::Settings binds = m3settings::defaults();
    std::string dir, guid;
    SDL_GameControllerType pad_type = SDL_CONTROLLER_TYPE_UNKNOWN;
};
State g;

Glyph read_glyph(int code) {
    Glyph out{};
    const std::uint8_t* o = g.rom + kFont + std::size_t(code) * 32;
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 16; ++x)
            if ((o[(x / 8) * 8 + y] >> (7 - x % 8)) & 1) out[y] |= std::uint16_t(1u << x);
    return out;
}

void write_glyph(int code, const Glyph& gl, int width) {
    std::uint8_t* o = g.rom + kFont + std::size_t(code) * 32;
    std::memset(o, 0, 32);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 16; ++x)
            if (gl[y] >> x & 1) o[(x / 8) * 8 + y] |= std::uint8_t(0x80 >> (x % 8));
    g.rom[kWidths + code] = static_cast<std::uint8_t>(width);
}

// Draws a label ("A", "LB", "icon:cross") into one glyph; returns its advance.
int render(const std::string& label, Glyph& out) {
    out = {};
    if (label.rfind("icon:", 0) == 0) {
        for (const Icon& ic : kIcons)
            if (label.compare(5, std::string::npos, ic.name) == 0) {
                for (int y = 0; y < 8; ++y)
                    for (int x = 0; ic.rows[y][x]; ++x)
                        if (ic.rows[y][x] == '#') out[y] |= std::uint16_t(1u << x);
                return 9;
            }
        return 1;
    }
    int x = 0;
    for (char c : label) {
        if (c < 32 || c > 126) continue;
        const int w = g.letter_width[c - 32];
        if (x + w > 17) break;  // a glyph is 16px wide
        for (int y = 0; y < 8; ++y) out[y] |= std::uint16_t(g.letters[c - 32][y] << x);
        x += w;
    }
    return x ? x : 1;
}

// SDL gamepad source ("a", "leftshoulder", "dpup") -> label for the pad style.
std::string pad_label(const std::string& src, Style s) {
    struct Row { const char *sdl, *xbox, *ps, *nin; };
    static const Row kRows[] = {
        {"a", "A", "icon:cross", "B"},       {"b", "B", "icon:circle", "A"},
        {"x", "X", "icon:square", "Y"},      {"y", "Y", "icon:triangle", "X"},
        {"leftshoulder", "LB", "L1", "L"},   {"rightshoulder", "RB", "R1", "R"},
        {"lefttrigger", "LT", "L2", "ZL"},   {"righttrigger", "RT", "R2", "ZR"},
        {"start", "icon:menu", "icon:menu", "icon:plus"},
        {"back", "icon:view", "icon:view", "icon:minus"},
        {"leftstick", "LS", "L3", "LS"},     {"rightstick", "RS", "R3", "RS"},
        {"dpup", "icon:up", "icon:up", "icon:up"},       {"dpdown", "icon:down", "icon:down", "icon:down"},
        {"dpleft", "icon:left", "icon:left", "icon:left"}, {"dpright", "icon:right", "icon:right", "icon:right"},
    };
    for (const Row& r : kRows)
        if (src == r.sdl) return s == kPlayStation ? r.ps : s == kNintendo ? r.nin : r.xbox;
    return src.substr(0, 2);
}

// SDL scancode name ("X", "Return", "Right Shift") -> label.
std::string key_label(const std::string& name) {
    if (name.size() == 1) return name;
    if (name == "Return" || name == "Keypad Enter") return "icon:return";
    if (name == "Left Shift" || name == "Right Shift") return "icon:shift";
    if (name == "Space") return "icon:space";
    if (name == "Up" || name == "Down" || name == "Left" || name == "Right") {
        std::string n = name;
        n[0] = static_cast<char>(n[0] - 'A' + 'a');
        return "icon:" + n;
    }
    return name.substr(0, 2);
}

Style style_of(SDL_GameControllerType t) {
    switch (t) {
        case SDL_CONTROLLER_TYPE_PS3: case SDL_CONTROLLER_TYPE_PS4: case SDL_CONTROLLER_TYPE_PS5:
            return kPlayStation;
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO: return kNintendo;
        default: return kXbox;
    }
}

void refresh_binds() {
    g.binds = m3settings::load(g.dir);
    g.guid.clear();
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
        if (SDL_IsGameController(i)) {
            char buf[64];
            SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), buf, sizeof buf);
            g.guid = buf;
            g.pad_type = SDL_GameControllerTypeForIndex(i);
            break;
        }
#if defined(GBAGAME_RECOMP_UI)
    if (!g.guid.empty()) rui_pad_binds_init(&kGbaPadSpec, (g.dir + "/input.ini").c_str());
#endif
}

std::string pad_source(int bind_index) {
#if defined(GBAGAME_RECOMP_UI)
    char out[64] = {};
    rui_pad_binds_label(&kGbaPadSpec, (g.dir + "/input.ini").c_str(), g.guid.c_str(), bind_index, out, sizeof out);
    return out;
#else
    static const char* const kDefaults[] = {"dpup", "dpdown", "dpleft", "dpright", "a",
                                            "b", "leftshoulder", "rightshoulder", "start", "back"};
    return kDefaults[bind_index];
#endif
}

// Which device the player touched last: any pad input beats nothing, any bound key wins back.
void poll_device() {
    if (!g.guid.empty()) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (!SDL_IsGameController(i)) continue;
            SDL_GameController* pad = SDL_GameControllerOpen(i);  // refcounted; the runtime holds it too
            if (!pad) continue;
            bool active = false;
            for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX && !active; ++b)
                active = SDL_GameControllerGetButton(pad, static_cast<SDL_GameControllerButton>(b));
            for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX && !active; ++a)
                active = std::abs(SDL_GameControllerGetAxis(pad, static_cast<SDL_GameControllerAxis>(a))) > 16000;
            SDL_GameControllerClose(pad);
            if (active) { g.style = style_of(g.pad_type); return; }
        }
    }
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    for (const std::string& k : g.binds.keys) {
        const SDL_Scancode sc = SDL_GetScancodeFromName(k.c_str());
        if (sc != SDL_SCANCODE_UNKNOWN && keys[sc]) { g.style = kKeyboard; return; }
    }
}

// Points every button mention in the script at the prompt glyphs.
void patch_script() {
    auto enc = [](const char* s) {
        std::string out;
        for (; *s; ++s) out += static_cast<char>(*s - 32);
        return out;
    };
    const std::string names[kCount] = {enc("A"), enc("B"), enc("L"), enc("R"), enc("Start"), enc("Select")};
    static const std::uint8_t kOpen[] = {0xF1, 0x05, 0x03, 0x00}, kClose[] = {0xF1, 0x05, 0x00, 0x00};
    int spans = 0;
    for (std::size_t p = kScript; p + 8 < kScriptEnd; ++p) {
        if (std::memcmp(g.rom + p, kOpen, 4) != 0) continue;
        for (int b = 0; b < kCount; ++b) {
            const std::string& n = names[b];
            const std::size_t at = p + 4;
            if (std::memcmp(g.rom + at, n.data(), n.size()) != 0 ||
                std::memcmp(g.rom + at + n.size(), kClose, 4) != 0)
                continue;
            g.rom[at] = kSlot[b];
            for (std::size_t k = 1; k < n.size(); ++k) g.rom[at + k] = kBlank;
            ++spans;
            break;
        }
    }
    // One tip names the shoulders without the highlight.
    const std::string plain = enc("L and R Button");
    for (std::size_t p = kScript; p + plain.size() < kScriptEnd; ++p)
        if (std::memcmp(g.rom + p, plain.data(), plain.size()) == 0) {
            g.rom[p] = kSlot[kL];
            g.rom[p + 6] = kSlot[kR];
            ++spans;
        }
    write_glyph(kBlank, Glyph{}, 0);
    SDL_Log("m3-prompts: %d button mentions now follow the controller", spans);
}

void redraw() {
    for (int b = 0; b < kCount; ++b) {
        const int i = kBindIndex[b];
        const std::string label = g.style == kTouch      ? kTouchLabels[b]
                                  : g.style == kKeyboard ? key_label(g.binds.keys[i])
                                                         : pad_label(pad_source(i), g.style);
        if (label == g.drawn[b]) continue;
        Glyph gl;
        const int advance = render(label, gl);
        write_glyph(kSlot[b], gl, advance);
        g.drawn[b] = label;
    }
}

}  // namespace

void attach(std::uint8_t* rom, std::size_t size, const char* source_sha1) {
    if (!rom || size != kRomSize || !source_sha1 ||
        (std::strcmp(source_sha1, kEnglishSha1) != 0 && std::strcmp(source_sha1, kHqAudioSha1) != 0))
        return;
    g.rom = rom;
    for (int c = 0; c < 0x5F; ++c) {
        g.letters[c] = read_glyph(c);
        g.letter_width[c] = rom[kWidths + c];
    }
    if (char* base = SDL_GetBasePath()) {
        g.dir = base;
        SDL_free(base);
        if (!g.dir.empty() && (g.dir.back() == '/' || g.dir.back() == '\\')) g.dir.pop_back();
    } else {
        g.dir = ".";
    }
}

void tick() {
    if (!g.rom) return;
    const Uint32 now = SDL_GetTicks();
    if (g.patched && now < g.next_poll) return;
    g.next_poll = now + 250;
    if (!g.patched || now >= g.next_binds) {
        refresh_binds();
        g.next_binds = now + 2000;
        if (!g.patched) g.style = g.guid.empty() ? kNoPad : style_of(g.pad_type);
    }
    if (!g.patched) {
        patch_script();
        g.patched = true;
    }
    if (g.binds.prompt_style > 0)  // 4 = keyboard (the on-screen buttons on Android)
        g.style = g.binds.prompt_style == 4 ? kNoPad : static_cast<Style>(g.binds.prompt_style);
    else poll_device();
    redraw();
}

}  // namespace m3prompts
