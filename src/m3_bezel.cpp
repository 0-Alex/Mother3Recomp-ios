#include "m3_bezel.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include "m3_assets.h"

namespace m3bezel {
namespace {

constexpr int kW = 1920, kH = 1080;
constexpr int kVersion = 2;  // bump when the layout changes so caches regenerate
constexpr int kSceneMap = 36, kSceneWidthTiles = 64;  // crossroads north of Tazmily
constexpr int kFrameScale = 3;                        // window tiles at 3x (24px)

struct Canvas {
    std::vector<std::uint32_t> px = std::vector<std::uint32_t>(std::size_t(kW) * kH, 0xFF000000u);
    // Nearest-neighbour blit of img at `scale`, skipping transparent pixels.
    void draw(const m3assets::Image& img, int x, int y, int scale) {
        for (int j = 0; j < img.h * scale; ++j)
            for (int i = 0; i < img.w * scale; ++i) {
                const std::uint32_t c = img.px[(j / scale) * img.w + i / scale];
                const int X = x + i, Y = y + j;
                if ((c >> 24) && X >= 0 && X < kW && Y >= 0 && Y < kH) px[std::size_t(Y) * kW + X] = c;
            }
    }
};

}  // namespace

void ensure(const std::uint8_t* rom_bytes, std::size_t size, const std::string& dir, int flavor) {
    namespace fs = std::filesystem;
    const fs::path folder = fs::path(dir) / "bezel_cache";
    const fs::path raw = folder / "m3_bezel_1080p.raw", key_file = folder / "m3_bezel.key";
    const std::string key = "v" + std::to_string(kVersion) + " flavor " + std::to_string(flavor);
    {
        std::ifstream k(key_file);
        std::string have;
        std::getline(k, have);
        std::error_code ec;
        if (have == key && fs::file_size(raw, ec) == std::uintmax_t(kW) * kH * 4) return;
    }
    m3assets::Rom rom(std::vector<std::uint8_t>(rom_bytes, rom_bytes + size));
    if (!rom.ok()) return;

    Canvas cv;
    // Scene at 2x, centred on the crossroads and darkened so the game stands out.
    const m3assets::Image scene = rom.map(kSceneMap, kSceneWidthTiles);
    if (scene.w > 0) cv.draw(scene, kW / 2 - 512 * 2, kH / 2 - 384 * 2, 2);
    for (std::uint32_t& c : cv.px) {
        const std::uint32_t r = (c >> 16 & 255) * 3 / 5, g = (c >> 8 & 255) * 3 / 5, b = (c & 255) * 3 / 5;
        c = 0xFF000000u | r << 16 | g << 8 | b;
    }

    // A menu window in the chosen flavor frames the game screen.
    constexpr int t = 8 * kFrameScale;
    const m3assets::Image frame = rom.window(kGameW / t + 2, kGameH / t + 2, flavor * 6);
    cv.draw(frame, kGameX - t, kGameY - t, kFrameScale);

    // The party stands in the side margins, facing the screen.
    struct Pose { int sprite, frame, x; };
    const Pose poses[] = {{4, 6, 70}, {13, 6, 160}, {15, 2, kW - 160}, {10, 2, kW - 70}};
    for (const Pose& p : poses)
        cv.draw(rom.sprite_frame(p.sprite, p.frame), p.x - 32 * 4, kH - 30 - 48 * 4, 4);

    std::error_code ec;
    fs::create_directories(folder, ec);
    std::ofstream out(raw, std::ios::binary | std::ios::trunc);
    std::vector<std::uint8_t> rgba(std::size_t(kW) * kH * 4);
    for (int y = 0; y < kH; ++y)
        for (int x = 0; x < kW; ++x) {
            const std::uint32_t c = cv.px[std::size_t(y) * kW + x];
            std::uint8_t* o = &rgba[(std::size_t(y) * kW + x) * 4];
            const bool hole = x >= kGameX && x < kGameX + kGameW && y >= kGameY && y < kGameY + kGameH;
            o[0] = c >> 16 & 255; o[1] = c >> 8 & 255; o[2] = c & 255; o[3] = hole ? 0 : 255;
        }
    out.write(reinterpret_cast<const char*>(rgba.data()), std::streamsize(rgba.size()));
    if (out) std::ofstream(key_file, std::ios::trunc) << key << "\n";
}

}  // namespace m3bezel
