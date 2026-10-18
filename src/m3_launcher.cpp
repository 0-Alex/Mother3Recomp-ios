#include "m3_launcher.h"

#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <string>
#include <vector>

namespace m3launcher {
namespace {

// 16:9 canvas; deliberately not 240x160 so the in-game bezel hook ignores it.
constexpr int kW = 320, kH = 180;
constexpr std::uint32_t kInk = 0xFF383040u;

// Scene: the crossroads by the bell tower north of Tazmily (map 36, 64 tiles wide).
constexpr int kSceneMap = 36, kSceneWidthTiles = 64;
struct Point { int x, y; };
// Feet positions along the dirt paths; the party loops through them forever.
constexpr Point kRoute[] = {{512, 384}, {512, 56}, {512, 384}, {992, 384},
                            {512, 384}, {512, 736}, {512, 384}, {32, 384}};
constexpr int kRouteLen = sizeof kRoute / sizeof kRoute[0];

struct Member { const char* name; int sprite; };
constexpr Member kParty[] = {{"Lucas", 4}, {"Kumatora", 13}, {"Duster", 15}, {"Boney", 10}};
constexpr int kPartySize = 4;
constexpr int kFollowGap = 18;  // frames of leader history between party members

// Standing frames run clockwise from facing down; walk cycles are +8 and +16.
enum Dir { kDown = 0, kLeft = 2, kUp = 4, kRight = 6 };

struct Canvas {
    std::vector<std::uint32_t> px = std::vector<std::uint32_t>(kW * kH);
    void fill(int x, int y, int w, int h, std::uint32_t c) {
        for (int j = std::max(y, 0); j < std::min(y + h, kH); ++j)
            for (int i = std::max(x, 0); i < std::min(x + w, kW); ++i) px[j * kW + i] = c;
    }
    // Draws img with its top-left at (x, y); tint != 0 recolours opaque pixels.
    void draw(const m3assets::Image& img, int x, int y, std::uint32_t tint = 0) {
        for (int j = std::max(0, -y); j < img.h && y + j < kH; ++j)
            for (int i = std::max(0, -x); i < img.w && x + i < kW; ++i) {
                const std::uint32_t c = img.px[j * img.w + i];
                if (c >> 24) px[(y + j) * kW + x + i] = tint ? tint : c;
            }
    }
    // Opaque blit of a kW x kH window of a larger image.
    void blit_view(const m3assets::Image& img, int sx, int sy) {
        for (int j = 0; j < kH && sy + j < img.h; ++j)
            std::copy_n(&img.px[(sy + j) * img.w + sx], std::min(kW, img.w - sx), &px[j * kW]);
    }
};

int text_width(const m3assets::Rom& rom, const std::string& s) {
    int w = 0, adv;
    for (char c : s) { rom.glyph(c, &adv); w += adv; }
    return w;
}

void text(Canvas& cv, const m3assets::Rom& rom, const std::string& s, int x, int y) {
    int adv;
    for (char c : s) {
        cv.draw(rom.glyph(c, &adv), x, y, kInk);
        x += adv;
    }
}

void cursor(Canvas& cv, int x, int y) {
    for (int k = 0; k < 4; ++k) cv.fill(x + k, y + k, 1, 7 - 2 * k, kInk);
}

const char* const kItems[] = {"Play", "Settings", "Controls", "Mods", "Quit"};
constexpr int kItemCount = 5;

enum class Input { None, Prev, Next, Confirm, Back };

Input map_key(SDL_Keycode k) {
    switch (k) {
        case SDLK_LEFT: case SDLK_UP: case SDLK_a: case SDLK_w: return Input::Prev;
        case SDLK_RIGHT: case SDLK_DOWN: case SDLK_d: case SDLK_s: return Input::Next;
        case SDLK_RETURN: case SDLK_x: case SDLK_SPACE: return Input::Confirm;
        case SDLK_ESCAPE: case SDLK_z: case SDLK_BACKSPACE: return Input::Back;
        default: return Input::None;
    }
}

Input map_button(Uint8 b) {
    switch (b) {
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: case SDL_CONTROLLER_BUTTON_DPAD_UP: return Input::Prev;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return Input::Next;
        case SDL_CONTROLLER_BUTTON_A: case SDL_CONTROLLER_BUTTON_START: return Input::Confirm;
        case SDL_CONTROLLER_BUTTON_B: case SDL_CONTROLLER_BUTTON_BACK: return Input::Back;
        default: return Input::None;
    }
}

struct Step { Point at; Dir dir; };

// Moves the leader 1px per frame toward the next route point.
struct Walker {
    Point at = kRoute[kRouteLen - 1];
    int target = 0;
    Dir dir = kRight;
    std::deque<Step> history;
    void tick() {
        const Point t = kRoute[target];
        if (at.x == t.x && at.y == t.y) target = (target + 1) % kRouteLen;
        const Point n = kRoute[target];
        if (n.x != at.x) { dir = n.x > at.x ? kRight : kLeft; at.x += n.x > at.x ? 1 : -1; }
        else if (n.y != at.y) { dir = n.y > at.y ? kDown : kUp; at.y += n.y > at.y ? 1 : -1; }
        history.push_front({at, dir});
        if (history.size() > std::size_t(kFollowGap * kPartySize)) history.pop_back();
    }
    Step member(int k) const {
        const std::size_t i = std::size_t(k * kFollowGap);
        return i < history.size() ? history[i] : (history.empty() ? Step{at, dir} : history.back());
    }
};

}  // namespace

Result run(const m3assets::Rom& rom) {
    const m3assets::Image scene = rom.map(kSceneMap, kSceneWidthTiles);
    if (scene.w < kW || scene.h < kH) return Result::Play;  // can't draw the launcher; just boot
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return Result::Play;
    SDL_Window* win = SDL_CreateWindow("MOTHER 3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       kW * 4, kH * 4, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    SDL_Texture* tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, kW, kH) : nullptr;
    if (!tex) {
        if (ren) SDL_DestroyRenderer(ren);
        if (win) SDL_DestroyWindow(win);
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
        return Result::Play;  // fall back to booting the game rather than blocking the player
    }
    SDL_RenderSetLogicalSize(ren, kW, kH);
    SDL_RenderSetIntegerScale(ren, SDL_TRUE);
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
        if (SDL_IsGameController(i)) SDL_GameControllerOpen(i);

    // Test hook: render a few seconds, save the canvas as BMP and quit.
    const char* shot = std::getenv("M3_LAUNCHER_SHOT");
    const m3assets::Image command_bar = rom.window(kW / 8, 3, 0);
    const m3assets::Image status_box = rom.window(kW / 8 / kPartySize, 4, 0);
    const m3assets::Image note = rom.window(24, 5, 0);
    std::vector<int> item_x;
    for (int i = 0, x = 18; i < kItemCount; ++i) {
        item_x.push_back(x);
        x += text_width(rom, kItems[i]) + 22;
    }

    Walker walker;
    int cursor_at = 0, frame = 0;
    bool sub_page = false, running = true;
    Result result = Result::Quit;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            Input in = Input::None;
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_KEYDOWN && !e.key.repeat) in = map_key(e.key.keysym.sym);
            else if (e.type == SDL_CONTROLLERBUTTONDOWN) in = map_button(e.cbutton.button);
            else if (e.type == SDL_CONTROLLERDEVICEADDED) SDL_GameControllerOpen(e.cdevice.which);
            if (sub_page) {
                if (in == Input::Confirm || in == Input::Back) sub_page = false;
                continue;
            }
            if (in == Input::Prev) cursor_at = (cursor_at + kItemCount - 1) % kItemCount;
            if (in == Input::Next) cursor_at = (cursor_at + 1) % kItemCount;
            if (in == Input::Back) cursor_at = kItemCount - 1;
            if (in == Input::Confirm) {
                if (cursor_at == 0) { result = Result::Play; running = false; }
                else if (cursor_at == kItemCount - 1) running = false;
                else sub_page = true;
            }
        }

        walker.tick();
        // Keep the leader centred in the space between the menu bars.
        const int cam_x = std::clamp(walker.at.x - kW / 2, 0, scene.w - kW);
        const int cam_y = std::clamp(walker.at.y - 8 - kH / 2, 0, scene.h - kH);
        Canvas cv;
        cv.blit_view(scene, cam_x, cam_y);

        // Back-to-front by feet position so nearer members overlap farther ones.
        std::vector<int> order(kPartySize);
        for (int k = 0; k < kPartySize; ++k) order[k] = kPartySize - 1 - k;
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
            return walker.member(a).at.y < walker.member(b).at.y;
        });
        const int walk_phase = (frame / 8) % 4;  // stand, step A, stand, step B
        for (int k : order) {
            const Step s = walker.member(k);
            const int f = s.dir + (walk_phase == 1 ? 8 : walk_phase == 3 ? 16 : 0);
            cv.draw(rom.sprite_frame(kParty[k].sprite, f), s.at.x - cam_x - 32, s.at.y - cam_y - 48);
        }

        cv.draw(command_bar, 0, 0);
        for (int i = 0; i < kItemCount; ++i) {
            if (i == cursor_at) cursor(cv, item_x[i] - 8, 8);
            text(cv, rom, kItems[i], item_x[i], 4);
        }
        for (int k = 0; k < kPartySize; ++k) {
            const int x = k * status_box.w, y = kH - status_box.h;
            cv.draw(status_box, x, y);
            text(cv, rom, kParty[k].name, x + (status_box.w - text_width(rom, kParty[k].name)) / 2, y + 8);
        }
        if (sub_page) {
            cv.draw(note, (kW - note.w) / 2, 70);
            const std::string msg = std::string(kItems[cursor_at]) + " is coming soon.";
            text(cv, rom, msg, (kW - text_width(rom, msg)) / 2, 82);
        }

        SDL_UpdateTexture(tex, nullptr, cv.px.data(), kW * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        ++frame;

        if (shot && frame == 150) {
            if (SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(cv.px.data(), kW, kH, 32, kW * 4,
                                                                    SDL_PIXELFORMAT_ARGB8888)) {
                SDL_SaveBMP(s, shot);
                SDL_FreeSurface(s);
            }
            result = Result::Quit;
            running = false;
        }
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    return result;
}

}  // namespace m3launcher
