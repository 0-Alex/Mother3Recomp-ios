#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int x, y, w, h;
} SDL_Rect;

typedef void* SDL_Renderer;
typedef void* SDL_Texture;

typedef int (*pfn_dynapi_entry)(uint32_t, void*, uint32_t);
typedef int (*pfn_SDL_GetRendererOutputSize)(SDL_Renderer* renderer, int *w, int *h);
typedef SDL_Texture* (*pfn_SDL_CreateTexture)(SDL_Renderer* renderer, uint32_t format, int access, int w, int h);
typedef int (*pfn_SDL_UpdateTexture)(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
typedef int (*pfn_SDL_RenderCopy)(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect);
typedef void (*pfn_SDL_RenderPresent)(SDL_Renderer* renderer);
typedef void (*pfn_SDL_DestroyTexture)(SDL_Texture* texture);
typedef int (*pfn_SDL_RenderSetViewport)(SDL_Renderer* renderer, const SDL_Rect* rect);
typedef int (*pfn_SDL_RenderSetClipRect)(SDL_Renderer* renderer, const SDL_Rect* rect);
typedef int (*pfn_SDL_RenderReadPixels)(SDL_Renderer* renderer, const SDL_Rect* rect, uint32_t format, void* pixels, int pitch);
typedef int (*pfn_SDL_SetTextureBlendMode)(SDL_Texture* texture, int blendMode);

typedef struct {
    uint32_t scancode;
    int32_t sym;
    uint16_t mod;
    uint32_t unused;
} SDL_Keysym_t;

typedef struct {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym_t keysym;
} SDL_KeyboardEvent_t;

typedef union {
    uint32_t type;
    SDL_KeyboardEvent_t key;
    uint8_t padding[56];
} SDL_Event_t;

typedef int (*pfn_SDL_PollEvent)(void* event);
typedef int (*pfn_SDL_WaitEvent)(void* event);
typedef int (*pfn_SDL_WaitEventTimeout)(void* event, int timeout);

static pfn_SDL_RenderPresent g_real_RenderPresent = NULL;
static pfn_SDL_RenderCopy g_real_RenderCopy = NULL;
static pfn_SDL_CreateTexture g_real_CreateTexture = NULL;
static pfn_SDL_UpdateTexture g_real_UpdateTexture = NULL;
static pfn_SDL_GetRendererOutputSize g_real_GetRendererOutputSize = NULL;
static pfn_SDL_DestroyTexture g_real_DestroyTexture = NULL;
static pfn_SDL_RenderSetViewport g_real_RenderSetViewport = NULL;
static pfn_SDL_RenderSetClipRect g_real_RenderSetClipRect = NULL;
static pfn_SDL_RenderReadPixels g_real_RenderReadPixels = NULL;
static pfn_SDL_SetTextureBlendMode g_real_SetTextureBlendMode = NULL;
static pfn_SDL_PollEvent g_real_PollEvent = NULL;
static pfn_SDL_WaitEvent g_real_WaitEvent = NULL;
static pfn_SDL_WaitEventTimeout g_real_WaitEventTimeout = NULL;

static SDL_Texture* g_tex_bezel = NULL;
static SDL_Renderer* g_last_renderer = NULL;
static uint8_t* g_bezel_pixels = NULL;
static int g_present_count = 0;

static int g_bezel_enabled = 1;
static int g_bezel_config_loaded = 0;

static void load_bezel_config(void) {
    if (g_bezel_config_loaded) return;
    g_bezel_config_loaded = 1;
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';
    
    char ini_path[MAX_PATH];
    snprintf(ini_path, MAX_PATH, "%s\\config.ini", exe_dir);
    int val = GetPrivateProfileIntA("Launcher", "bezel", 1, ini_path);
    g_bezel_enabled = (val != 0);
}

static void log_hotkey(const char* msg) {
    if (!getenv("MOTHER3_DEBUG_HOTKEY")) return;
    FILE* f = fopen("hotkey.log", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

// 8x8 bitmap font (ASCII 32 to 126) for on-screen HUD
static const uint8_t g_font8x8[95][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ' ' */
    {0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00}, /* '!' */
    {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '"' */
    {0x10, 0x78, 0x10, 0x78, 0x20, 0x00, 0x00, 0x00}, /* '#' */
    {0x38, 0x50, 0x30, 0x28, 0x78, 0x20, 0x00, 0x00}, /* '$' */
    {0x78, 0x10, 0x20, 0x3c, 0x5c, 0x00, 0x00, 0x00}, /* '%' */
    {0x50, 0x60, 0x68, 0x50, 0x78, 0x00, 0x00, 0x00}, /* '&' */
    {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* ''' */
    {0x10, 0x20, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00}, /* '(' */
    {0x10, 0x08, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00}, /* ')' */
    {0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '*' */
    {0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, 0x00}, /* '+' */
    {0x00, 0x00, 0x00, 0x00, 0x10, 0x30, 0x00, 0x00}, /* ',' */
    {0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '-' */
    {0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, /* '.' */
    {0x10, 0x10, 0x20, 0x20, 0x40, 0x40, 0x00, 0x00}, /* '/' */
    {0x30, 0x58, 0x78, 0x68, 0x30, 0x00, 0x00, 0x00}, /* '0' */
    {0x30, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* '1' */
    {0x38, 0x28, 0x08, 0x10, 0x38, 0x00, 0x00, 0x00}, /* '2' */
    {0x38, 0x08, 0x18, 0x08, 0x38, 0x00, 0x00, 0x00}, /* '3' */
    {0x30, 0x30, 0x50, 0x78, 0x10, 0x00, 0x00, 0x00}, /* '4' */
    {0x38, 0x20, 0x38, 0x08, 0x30, 0x00, 0x00, 0x00}, /* '5' */
    {0x38, 0x40, 0x78, 0x48, 0x30, 0x00, 0x00, 0x00}, /* '6' */
    {0x70, 0x10, 0x20, 0x20, 0x40, 0x00, 0x00, 0x00}, /* '7' */
    {0x78, 0x48, 0x30, 0x48, 0x78, 0x00, 0x00, 0x00}, /* '8' */
    {0x30, 0x48, 0x78, 0x08, 0x70, 0x00, 0x00, 0x00}, /* '9' */
    {0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00}, /* ':' */
    {0x00, 0x10, 0x00, 0x00, 0x10, 0x30, 0x00, 0x00}, /* ';' */
    {0x00, 0x08, 0x10, 0x30, 0x08, 0x00, 0x00, 0x00}, /* '<' */
    {0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00}, /* '=' */
    {0x00, 0x20, 0x10, 0x18, 0x20, 0x00, 0x00, 0x00}, /* '>' */
    {0x08, 0x08, 0x38, 0x00, 0x20, 0x00, 0x00, 0x00}, /* '?' */
    {0x48, 0xb8, 0xb8, 0xb8, 0xa8, 0x80, 0x70, 0x00}, /* '@' */
    {0x10, 0x28, 0x28, 0x38, 0x48, 0x00, 0x00, 0x00}, /* 'A' */
    {0x3c, 0x24, 0x38, 0x24, 0x3c, 0x00, 0x00, 0x00}, /* 'B' */
    {0x30, 0x40, 0x40, 0x40, 0x30, 0x00, 0x00, 0x00}, /* 'C' */
    {0x70, 0x48, 0x48, 0x48, 0x70, 0x00, 0x00, 0x00}, /* 'D' */
    {0x38, 0x20, 0x38, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 'E' */
    {0x38, 0x20, 0x38, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'F' */
    {0x38, 0x40, 0x58, 0x48, 0x38, 0x00, 0x00, 0x00}, /* 'G' */
    {0x48, 0x48, 0x78, 0x48, 0x48, 0x00, 0x00, 0x00}, /* 'H' */
    {0x38, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'I' */
    {0x38, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x00}, /* 'J' */
    {0x28, 0x30, 0x30, 0x30, 0x28, 0x00, 0x00, 0x00}, /* 'K' */
    {0x20, 0x20, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 'L' */
    {0x30, 0x30, 0x68, 0x48, 0x48, 0x00, 0x00, 0x00}, /* 'M' */
    {0x48, 0x68, 0x68, 0x58, 0x48, 0x00, 0x00, 0x00}, /* 'N' */
    {0x30, 0x48, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00}, /* 'O' */
    {0x3c, 0x24, 0x38, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'P' */
    {0x30, 0x48, 0x48, 0x48, 0x30, 0x18, 0x00, 0x00}, /* 'Q' */
    {0x38, 0x28, 0x30, 0x28, 0x28, 0x00, 0x00, 0x00}, /* 'R' */
    {0x3c, 0x20, 0x18, 0x04, 0x3c, 0x00, 0x00, 0x00}, /* 'S' */
    {0x7c, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00}, /* 'T' */
    {0x48, 0x48, 0x48, 0x48, 0x70, 0x00, 0x00, 0x00}, /* 'U' */
    {0x28, 0x28, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00}, /* 'V' */
    {0x48, 0x48, 0x68, 0x78, 0x48, 0x00, 0x00, 0x00}, /* 'W' */
    {0x48, 0x30, 0x20, 0x30, 0x48, 0x00, 0x00, 0x00}, /* 'X' */
    {0x44, 0x28, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00}, /* 'Y' */
    {0x70, 0x10, 0x20, 0x40, 0x78, 0x00, 0x00, 0x00}, /* 'Z' */
    {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x00}, /* '[' */
    {0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x00, 0x00}, /* '\' */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0x00}, /* ']' */
    {0x20, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '^' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00}, /* '_' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* '`' */
    {0x00, 0x3c, 0x3c, 0x24, 0x3c, 0x00, 0x00, 0x00}, /* 'a' */
    {0x20, 0x3c, 0x24, 0x24, 0x38, 0x00, 0x00, 0x00}, /* 'b' */
    {0x00, 0x18, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 'c' */
    {0x08, 0x38, 0x48, 0x48, 0x78, 0x00, 0x00, 0x00}, /* 'd' */
    {0x00, 0x38, 0x78, 0x40, 0x38, 0x00, 0x00, 0x00}, /* 'e' */
    {0x20, 0x70, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'f' */
    {0x00, 0x78, 0x48, 0x78, 0x78, 0x48, 0x70, 0x00}, /* 'g' */
    {0x20, 0x38, 0x28, 0x28, 0x28, 0x00, 0x00, 0x00}, /* 'h' */
    {0x00, 0x30, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'i' */
    {0x00, 0x38, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00}, /* 'j' */
    {0x20, 0x28, 0x30, 0x28, 0x24, 0x00, 0x00, 0x00}, /* 'k' */
    {0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'l' */
    {0x00, 0x7c, 0x54, 0x54, 0x54, 0x00, 0x00, 0x00}, /* 'm' */
    {0x00, 0x38, 0x28, 0x28, 0x28, 0x00, 0x00, 0x00}, /* 'n' */
    {0x00, 0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00}, /* 'o' */
    {0x00, 0x38, 0x28, 0x28, 0x38, 0x20, 0x20, 0x00}, /* 'p' */
    {0x00, 0x38, 0x48, 0x48, 0x78, 0x08, 0x08, 0x00}, /* 'q' */
    {0x00, 0x3c, 0x24, 0x20, 0x20, 0x00, 0x00, 0x00}, /* 'r' */
    {0x00, 0x38, 0x20, 0x18, 0x38, 0x00, 0x00, 0x00}, /* 's' */
    {0x20, 0x78, 0x20, 0x20, 0x38, 0x00, 0x00, 0x00}, /* 't' */
    {0x00, 0x28, 0x28, 0x28, 0x38, 0x00, 0x00, 0x00}, /* 'u' */
    {0x00, 0x28, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00}, /* 'v' */
    {0x00, 0x88, 0xb0, 0x70, 0x50, 0x00, 0x00, 0x00}, /* 'w' */
    {0x00, 0x48, 0x30, 0x30, 0x48, 0x00, 0x00, 0x00}, /* 'x' */
    {0x00, 0x28, 0x28, 0x28, 0x10, 0x10, 0x60, 0x00}, /* 'y' */
    {0x00, 0x38, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00}, /* 'z' */
    {0x10, 0x10, 0x20, 0x10, 0x10, 0x10, 0x18, 0x00}, /* '{' */
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00}, /* '|' */
    {0x10, 0x10, 0x08, 0x10, 0x10, 0x10, 0x30, 0x00}, /* '}' */
    {0x00, 0x00, 0xd0, 0xb0, 0x00, 0x00, 0x00, 0x00}, /* '~' */
};

#define TOAST_W 720
#define TOAST_H 44
static uint32_t g_toast_pixels[TOAST_W * TOAST_H];
static SDL_Texture* g_tex_toast = NULL;
static DWORD g_toast_expiry = 0;
static char g_toast_msg[128] = {0};

static void draw_toast_text(const char* text) {
    for (int y = 0; y < TOAST_H; y++) {
        for (int x = 0; x < TOAST_W; x++) {
            if (y == 0 || y == TOAST_H - 1 || x == 0 || x == TOAST_W - 1 ||
                y == 1 || y == TOAST_H - 2 || x == 1 || x == TOAST_W - 2) {
                g_toast_pixels[y * TOAST_W + x] = 0xFFF0A000; // Amber Gold Border
            } else {
                g_toast_pixels[y * TOAST_W + x] = 0xEE1E1E1E; // Dark charcoal pill
            }
        }
    }

    int len = (int)strlen(text);
    int start_x = (TOAST_W - (len * 12)) / 2;
    if (start_x < 10) start_x = 10;
    int start_y = (TOAST_H - 16) / 2;

    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c < 32 || c > 126) c = ' ';
        int glyph_idx = c - 32;
        int cx = start_x + (i * 12);
        for (int gy = 0; gy < 8; gy++) {
            uint8_t row = g_font8x8[glyph_idx][gy];
            for (int gx = 0; gx < 8; gx++) {
                if (row & (1 << (7 - gx))) {
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int px = cx + (gx + dx);
                            int py = start_y + (gy * 2 + dy);
                            if (px >= 0 && px < TOAST_W && py >= 0 && py < TOAST_H) {
                                g_toast_pixels[py * TOAST_W + px] = 0xFFFFFFFF; // Clean White Text
                            }
                        }
                    }
                }
            }
        }
    }
}

static int g_gamepad_config_loaded = 0;
static FILETIME g_last_keybinds_time = {0, 0};
static uint8_t g_btn_map[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
static int g_trigger_l_target = 9;   // Default: LB (button 9)
static int g_trigger_r_target = 10;  // Default: RB (button 10)
static int g_lt_active = 0;
static int g_rt_active = 0;
static int g_current_preset = 0;

static uint8_t parse_btn_target(const char* name, uint8_t def) {
    if (!_stricmp(name, "none") || !_stricmp(name, "unbound") || !_stricmp(name, "off")) return 255;
    if (!_stricmp(name, "a")) return 0;
    if (!_stricmp(name, "b")) return 1;
    if (!_stricmp(name, "x")) return 2;
    if (!_stricmp(name, "y")) return 3;
    if (!_stricmp(name, "back") || !_stricmp(name, "select")) return 4;
    if (!_stricmp(name, "guide")) return 5;
    if (!_stricmp(name, "start")) return 6;
    if (!_stricmp(name, "l3") || !_stricmp(name, "leftstick")) return 7;
    if (!_stricmp(name, "r3") || !_stricmp(name, "rightstick")) return 8;
    if (!_stricmp(name, "l") || !_stricmp(name, "lb") || !_stricmp(name, "leftshoulder")) return 9;
    if (!_stricmp(name, "r") || !_stricmp(name, "rb") || !_stricmp(name, "rightshoulder")) return 10;
    if (!_stricmp(name, "up") || !_stricmp(name, "dpup")) return 11;
    if (!_stricmp(name, "down") || !_stricmp(name, "dpdown")) return 12;
    if (!_stricmp(name, "left") || !_stricmp(name, "dpleft")) return 13;
    if (!_stricmp(name, "right") || !_stricmp(name, "dpright")) return 14;
    return def;
}

static int parse_trigger_target(const char* val, int def_target) {
    if (!_stricmp(val, "false") || !_stricmp(val, "0") || !_stricmp(val, "none") || !_stricmp(val, "off")) return -1;
    if (!_stricmp(val, "true") || !_stricmp(val, "1")) return def_target;
    if (!_stricmp(val, "l") || !_stricmp(val, "lb") || !_stricmp(val, "leftshoulder")) return 9;
    if (!_stricmp(val, "r") || !_stricmp(val, "rb") || !_stricmp(val, "rightshoulder")) return 10;
    if (!_stricmp(val, "a")) return 0;
    if (!_stricmp(val, "b") || !_stricmp(val, "dash")) return 1;
    return def_target;
}

static void load_gamepad_config(void) {
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';

    char ini_path[MAX_PATH];
    snprintf(ini_path, MAX_PATH, "%s\\keybinds.ini", exe_dir);

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(ini_path, GetFileExInfoStandard, &fad)) {
        if (g_gamepad_config_loaded &&
            fad.ftLastWriteTime.dwLowDateTime == g_last_keybinds_time.dwLowDateTime &&
            fad.ftLastWriteTime.dwHighDateTime == g_last_keybinds_time.dwHighDateTime) {
            return;
        }
        g_last_keybinds_time = fad.ftLastWriteTime;
    } else if (g_gamepad_config_loaded) {
        return;
    }
    g_gamepad_config_loaded = 1;

    for (int i = 0; i < 15; i++) g_btn_map[i] = (uint8_t)i;
    g_btn_map[2] = 1; // Xbox X defaults to Dash (GBA B)
    g_btn_map[3] = 0; // Xbox Y defaults to Confirm (GBA A)

    char layout[64];
    layout[0] = '\0';
    GetPrivateProfileStringA("gamepad1", "layout", "xbox", layout, sizeof(layout), ini_path);

    if (!_stricmp(layout, "nintendo") || !_stricmp(layout, "switch") || !_stricmp(layout, "snes")) {
        g_btn_map[0] = 1; // Xbox A -> GBA B (Dash)
        g_btn_map[1] = 0; // Xbox B -> GBA A (Confirm)
        g_btn_map[2] = 0; // Xbox X -> GBA A (Confirm)
        g_btn_map[3] = 1; // Xbox Y -> GBA B (Dash)
        g_current_preset = 1;
    } else if (!_stricmp(layout, "classic")) {
        g_btn_map[2] = 2; // Leave X as X
        g_btn_map[3] = 3; // Leave Y as Y
        g_current_preset = 3;
    } else {
        g_current_preset = 0;
    }

    char val[64];
    #define OVERRIDE_BTN(key, btn_idx) \
        if (GetPrivateProfileStringA("gamepad1", key, "", val, sizeof(val), ini_path) > 0) \
            g_btn_map[btn_idx] = parse_btn_target(val, g_btn_map[btn_idx]);

    OVERRIDE_BTN("a", 0);
    OVERRIDE_BTN("b", 1);
    OVERRIDE_BTN("x", 2);
    OVERRIDE_BTN("y", 3);
    OVERRIDE_BTN("l", 9);
    OVERRIDE_BTN("lb", 9);
    OVERRIDE_BTN("r", 10);
    OVERRIDE_BTN("rb", 10);
    OVERRIDE_BTN("select", 4);
    OVERRIDE_BTN("back", 4);
    OVERRIDE_BTN("start", 6);
    OVERRIDE_BTN("up", 11);
    OVERRIDE_BTN("down", 12);
    OVERRIDE_BTN("left", 13);
    OVERRIDE_BTN("right", 14);
    #undef OVERRIDE_BTN

    char trig_l[32], trig_r[32];
    GetPrivateProfileStringA("gamepad1", "trigger_l", "true", trig_l, sizeof(trig_l), ini_path);
    GetPrivateProfileStringA("gamepad1", "trigger_r", "true", trig_r, sizeof(trig_r), ini_path);
    g_trigger_l_target = parse_trigger_target(trig_l, 9);
    g_trigger_r_target = parse_trigger_target(trig_r, 10);
    if (g_trigger_r_target == 1 && g_current_preset == 0) {
        g_current_preset = 2; // Modern Trigger Dash
    }

    char log_buf[256];
    snprintf(log_buf, sizeof(log_buf),
        "[GAMEPAD] Config loaded: A->%d, B->%d, X->%d, Y->%d, LB->%d, RB->%d, LT->%d, RT->%d",
        g_btn_map[0], g_btn_map[1], g_btn_map[2], g_btn_map[3],
        g_btn_map[9], g_btn_map[10], g_trigger_l_target, g_trigger_r_target);
    log_hotkey(log_buf);
}

// GBA Action Row indices (0 to 9 in launcher table)
// Row 0: Confirm (A) -> GBA target 0
// Row 1: Cancel / Run (B) -> GBA target 1
// Row 2: Select -> GBA target 4
// Row 3: Start -> GBA target 6
// Row 4: D-Pad Right -> GBA target 14
// Row 5: D-Pad Left -> GBA target 13
// Row 6: D-Pad Up -> GBA target 11
// Row 7: D-Pad Down -> GBA target 12
// Row 8: R Shoulder -> GBA target 10
// Row 9: L Shoulder -> GBA target 9
static const uint8_t s_capture_to_gba[12] = {
    0,   // Row 0: Confirm (A)
    1,   // Row 1: Cancel / Run (B)
    4,   // Row 2: Select
    6,   // Row 3: Start
    14,  // Row 4: D-Pad Right
    13,  // Row 5: D-Pad Left
    11,  // Row 6: D-Pad Up
    12,  // Row 7: D-Pad Down
    10,  // Row 8: R Shoulder
    9,   // Row 9: L Shoulder
    255, // Row 10: Reserved
    255  // Row 11: Reserved
};

static const char* target_to_str(uint8_t target) {
    switch (target) {
        case 0: return "a";
        case 1: return "b";
        case 2: return "x";
        case 3: return "y";
        case 4: return "select";
        case 6: return "start";
        case 9: return "l";
        case 10: return "r";
        case 11: return "up";
        case 12: return "down";
        case 13: return "left";
        case 14: return "right";
        case 255: return "none";
        default: return "none";
    }
}

static void save_gamepad_config(void) {
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';

    char ini_path[MAX_PATH];
    snprintf(ini_path, MAX_PATH, "%s\\keybinds.ini", exe_dir);

    const char* layout_str = "xbox";
    if (g_current_preset == 1) layout_str = "nintendo";
    else if (g_current_preset == 3) layout_str = "classic";

    WritePrivateProfileStringA("gamepad1", "layout", layout_str, ini_path);
    WritePrivateProfileStringA("gamepad1", "a", target_to_str(g_btn_map[0]), ini_path);
    WritePrivateProfileStringA("gamepad1", "b", target_to_str(g_btn_map[1]), ini_path);
    WritePrivateProfileStringA("gamepad1", "x", target_to_str(g_btn_map[2]), ini_path);
    WritePrivateProfileStringA("gamepad1", "y", target_to_str(g_btn_map[3]), ini_path);
    WritePrivateProfileStringA("gamepad1", "lb", target_to_str(g_btn_map[9]), ini_path);
    WritePrivateProfileStringA("gamepad1", "rb", target_to_str(g_btn_map[10]), ini_path);
    WritePrivateProfileStringA("gamepad1", "select", target_to_str(g_btn_map[4]), ini_path);
    WritePrivateProfileStringA("gamepad1", "start", target_to_str(g_btn_map[6]), ini_path);
    WritePrivateProfileStringA("gamepad1", "up", target_to_str(g_btn_map[11]), ini_path);
    WritePrivateProfileStringA("gamepad1", "down", target_to_str(g_btn_map[12]), ini_path);
    WritePrivateProfileStringA("gamepad1", "left", target_to_str(g_btn_map[13]), ini_path);
    WritePrivateProfileStringA("gamepad1", "right", target_to_str(g_btn_map[14]), ini_path);

    if (g_trigger_l_target >= 0) {
        WritePrivateProfileStringA("gamepad1", "trigger_l", target_to_str((uint8_t)g_trigger_l_target), ini_path);
    } else {
        WritePrivateProfileStringA("gamepad1", "trigger_l", "none", ini_path);
    }
    if (g_trigger_r_target >= 0) {
        if (g_trigger_r_target == 1) {
            WritePrivateProfileStringA("gamepad1", "trigger_r", "dash", ini_path);
        } else {
            WritePrivateProfileStringA("gamepad1", "trigger_r", target_to_str((uint8_t)g_trigger_r_target), ini_path);
        }
    } else {
        WritePrivateProfileStringA("gamepad1", "trigger_r", "none", ini_path);
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExA(ini_path, GetFileExInfoStandard, &fad)) {
        g_last_keybinds_time = fad.ftLastWriteTime;
    }

    log_hotkey("[GAMEPAD] Saved custom bindings to keybinds.ini [gamepad1]");
}

static const char* get_action_label(int act_idx) {
    uint8_t gba_act = s_capture_to_gba[act_idx];
    if (gba_act == 255) return "(none)";

    if (act_idx == 8 && g_trigger_r_target == 10) return "RB / RT";
    if (act_idx == 9 && g_trigger_l_target == 9) return "LB / LT";
    if (act_idx == 1 && g_trigger_r_target == 1) return "B / RT";

    for (int p = 0; p < 15; p++) {
        if (g_btn_map[p] == gba_act) {
            switch (p) {
                case 0: return "A";
                case 1: return "B";
                case 2: return "X";
                case 3: return "Y";
                case 4: return "Back";
                case 6: return "Start";
                case 9: return "LB";
                case 10: return "RB";
                case 11: return "DpadUp";
                case 12: return "DpadDown";
                case 13: return "DpadLeft";
                case 14: return "DpadRight";
                default: break;
            }
        }
    }
    return "(unbound)";
}

static void sync_launcher_gamepad_strings(void* m) {
    if (!m) return;
    uint8_t* pm = (uint8_t*)m;
    for (int i = 0; i < 10; i++) {
        const char* label = get_action_label(i);
        char* dst = (char*)(pm + (i * 32) + 0xf1cc);
        strncpy(dst, label, 31);
        dst[31] = '\0';
        *(uint8_t*)(pm + (i * 32) + 0xf1c0 + 0xc) = 1;
    }
}

static void* g_active_launcher_model = NULL;
static int g_active_capture_btn = -1;

void Hooked_launcher_model_begin_capture_gamepad(void* m, int b) {
    if (!m || b > 11) return;
    g_active_launcher_model = m;
    g_active_capture_btn = b;

    uint8_t* pm = (uint8_t*)m;
    pm[0xc1a8] = 1;
    *(uint32_t*)(pm + 0xc1ac) = (uint32_t)b;
    pm[0xc1b0] = 0;
    pm[0xc1b4] = 1;
    pm[0xc1bc] = 1;
    pm[0xc1bd] = 0;
    pm[0xc1c4] = 0;

    char log[128];
    snprintf(log, sizeof(log), "[LAUNCHER] Gamepad capture started for action row %d", b);
    log_hotkey(log);
}

void Hooked_launcher_model_begin_capture(void* m, int b) {
    if (!m || b > 11) return;
    g_active_launcher_model = m;
    g_active_capture_btn = b;

    uint8_t* pm = (uint8_t*)m;
    pm[0xc1a8] = 0;
    pm[0xc1b4] = 0;
    pm[0xc1c5] = 0;
    pm[0xc1bd] = 1;
    *(uint32_t*)(pm + 0xc1c0) = (uint32_t)b;

    char log[128];
    snprintf(log, sizeof(log), "[LAUNCHER] Keyboard capture started for button index %d", b);
    log_hotkey(log);
}

typedef void (*pfn_launcher_model_frame)(void* m);
static pfn_launcher_model_frame g_trampoline_launcher_model_frame = NULL;
static uint8_t g_tramp_frame_buf[32];

void Hooked_launcher_model_frame(void* m) {
    if (m) {
        g_active_launcher_model = m;
        uint8_t* pm = (uint8_t*)m;
        // When input source is Gamepad (index 2)
        if (*(uint32_t*)(pm + 0x4ea8) == 2) {
            pm[0x6b40] = 1;
            if (pm[0xc1a8] == 0 && pm[0xc1bd] == 0) {
                sync_launcher_gamepad_strings(m);
            }
        }
    }
    if (g_trampoline_launcher_model_frame) {
        g_trampoline_launcher_model_frame(m);
    }
}

static void apply_preset(int preset) {
    g_current_preset = (preset % 4 + 4) % 4;
    for (int i = 0; i < 15; i++) g_btn_map[i] = (uint8_t)i;

    switch (g_current_preset) {
        case 0: // Xbox / PC Standard
            g_btn_map[2] = 1; // X -> Dash (B)
            g_btn_map[3] = 0; // Y -> Confirm (A)
            g_trigger_l_target = 9;  // LT -> L
            g_trigger_r_target = 10; // RT -> R
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Xbox Standard (A=Confirm, B/X=Dash, RT=R)");
            break;
        case 1: // Nintendo / Switch Pro
            g_btn_map[0] = 1; // A -> Dash (B)
            g_btn_map[1] = 0; // B -> Confirm (A)
            g_btn_map[2] = 0; // X -> Confirm (A)
            g_btn_map[3] = 1; // Y -> Dash (B)
            g_trigger_l_target = 9;
            g_trigger_r_target = 10;
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Nintendo Switch (B=Confirm, A/Y=Dash)");
            break;
        case 2: // Modern Trigger Dash
            g_btn_map[2] = 1; // X -> Dash
            g_btn_map[3] = 0; // Y -> Confirm
            g_trigger_l_target = 9;
            g_trigger_r_target = 1;  // RT -> Dash (B)!
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Trigger Dash (A=Confirm, B=Cancel, RT=Dash!)");
            break;
        case 3: // Classic 1:1 Hardware
            g_trigger_l_target = 9;
            g_trigger_r_target = 10;
            snprintf(g_toast_msg, sizeof(g_toast_msg), "PRESET: Classic 1:1 Hardware (A=A, B=B, X=X, Y=Y)");
            break;
    }
    save_gamepad_config();
    if (g_active_launcher_model) {
        sync_launcher_gamepad_strings(g_active_launcher_model);
    }
    g_toast_expiry = GetTickCount() + 3500;
    log_hotkey(g_toast_msg);
}

static int get_pe_text_section(uint8_t* pBase, uint8_t** out_start, size_t* out_size) {
    if (!pBase || pBase[0] != 'M' || pBase[1] != 'Z') return 0;
    uint32_t lfanew = *(uint32_t*)(pBase + 0x3C);
    if (lfanew == 0 || lfanew > 0x1000) return 0;
    uint8_t* nt = pBase + lfanew;
    if (nt[0] != 'P' || nt[1] != 'E' || nt[2] != 0 || nt[3] != 0) return 0;
    uint16_t num_sec = *(uint16_t*)(nt + 6);
    uint16_t opt_size = *(uint16_t*)(nt + 20);
    uint8_t* sec = nt + 24 + opt_size;
    for (uint16_t i = 0; i < num_sec; ++i, sec += 40) {
        uint32_t chars = *(uint32_t*)(sec + 36);
        if ((chars & 0x20000020u) != 0) { // IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE
            uint32_t vsize = *(uint32_t*)(sec + 8);
            uint32_t rva = *(uint32_t*)(sec + 12);
            *out_start = pBase + rva;
            *out_size = (size_t)vsize;
            return 1;
        }
    }
    return 0;
}

static uint8_t* find_pe_pattern(uint8_t* start, size_t size, const uint8_t* pat, const char* mask, size_t pat_len) {
    if (!start || size < pat_len || pat_len == 0) return NULL;
    size_t limit = size - pat_len;
    uint8_t first = pat[0];
    for (size_t i = 0; i <= limit; ++i) {
        if (mask[0] == 'x' && start[i] != first) continue;
        size_t j = 1;
        for (; j < pat_len; ++j) {
            if (mask[j] == 'x' && start[i + j] != pat[j]) break;
        }
        if (j == pat_len) return start + i;
    }
    return NULL;
}

static uint8_t* resolve_rva_or_pattern(uint8_t* pBase, size_t rva, const uint8_t* pat, const char* mask, size_t pat_len) {
    uint8_t* text_start = NULL;
    size_t text_size = 0;
    if (get_pe_text_section(pBase, &text_start, &text_size)) {
        uint8_t* cand = pBase + rva;
        if (cand >= text_start && cand + pat_len <= text_start + text_size) {
            size_t j = 0;
            for (; j < pat_len; ++j) {
                if (mask[j] == 'x' && cand[j] != pat[j]) break;
            }
            if (j == pat_len) return cand;
        }
        return find_pe_pattern(text_start, text_size, pat, mask, pat_len);
    }
    return pBase + rva;
}

static void install_launcher_hooks(uint8_t* pBase) {
    // 1. Unhide GAMEPAD BINDINGS table UI (NOP out je 0x3948744)
    uint8_t* pPadBindsJump = pBase + 0x394ab40;
    if (pPadBindsJump[0] == 0x0f && pPadBindsJump[1] == 0x84) {
        DWORD oldProtect;
        if (VirtualProtect(pPadBindsJump, 6, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memset(pPadBindsJump, 0x90, 6);
            VirtualProtect(pPadBindsJump, 6, oldProtect, &oldProtect);
            log_hotkey("[LAUNCHER] Unlocked GAMEPAD BINDINGS table");
        }
    }

    // 2. Unhide Gamepad Profile options header (NOP out je 0x3948f79)
    uint8_t* pPadOptsJump = pBase + 0x394a22d;
    if (pPadOptsJump[0] == 0x0f && pPadOptsJump[1] == 0x84) {
        DWORD oldProtect;
        if (VirtualProtect(pPadOptsJump, 6, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memset(pPadOptsJump, 0x90, 6);
            VirtualProtect(pPadOptsJump, 6, oldProtect, &oldProtect);
            log_hotkey("[LAUNCHER] Unlocked Gamepad Profile header");
        }
    }

    // 3. Hook launcher_model_begin_capture_gamepad (RVA 0x3a47f80 or pattern fallback)
    static const uint8_t kPatPadCap[] = { 0xb8, 0x0c, 0x00, 0x00, 0x00 };
    uint8_t* pPadCapTarget = resolve_rva_or_pattern(pBase, 0x3a47f80, kPatPadCap, "xxxxx", sizeof(kPatPadCap));
    if (pPadCapTarget && pPadCapTarget[0] == 0xb8 && pPadCapTarget[1] == 0x0c) {
        DWORD oldProtect;
        if (VirtualProtect(pPadCapTarget, 15, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            pPadCapTarget[0] = 0xFF;
            pPadCapTarget[1] = 0x25;
            pPadCapTarget[2] = 0x00;
            pPadCapTarget[3] = 0x00;
            pPadCapTarget[4] = 0x00;
            pPadCapTarget[5] = 0x00;
            *(uint64_t*)(pPadCapTarget + 6) = (uint64_t)(uintptr_t)Hooked_launcher_model_begin_capture_gamepad;
            pPadCapTarget[14] = 0x90;
            VirtualProtect(pPadCapTarget, 15, oldProtect, &oldProtect);
            log_hotkey("[LAUNCHER] Hooked launcher_model_begin_capture_gamepad successfully");
        }
    }

    // 4. Hook launcher_model_begin_capture (RVA 0x3a48710 or pattern fallback)
    static const uint8_t kPatKeyCap[] = { 0x48, 0x85, 0xc9, 0x74, 0x27 };
    uint8_t* pKeyCapTarget = resolve_rva_or_pattern(pBase, 0x3a48710, kPatKeyCap, "xxxxx", sizeof(kPatKeyCap));
    if (pKeyCapTarget && pKeyCapTarget[0] == 0x48 && pKeyCapTarget[1] == 0x85 && pKeyCapTarget[2] == 0xc9 && pKeyCapTarget[3] == 0x74 && pKeyCapTarget[4] == 0x27) {
        DWORD oldProtect;
        if (VirtualProtect(pKeyCapTarget, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            pKeyCapTarget[0] = 0xFF;
            pKeyCapTarget[1] = 0x25;
            pKeyCapTarget[2] = 0x00;
            pKeyCapTarget[3] = 0x00;
            pKeyCapTarget[4] = 0x00;
            pKeyCapTarget[5] = 0x00;
            *(uint64_t*)(pKeyCapTarget + 6) = (uint64_t)(uintptr_t)Hooked_launcher_model_begin_capture;
            VirtualProtect(pKeyCapTarget, 14, oldProtect, &oldProtect);
            log_hotkey("[LAUNCHER] Hooked launcher_model_begin_capture successfully");
        }
    }

    // 5. Hook launcher_model_frame (RVA 0x3a48b00 or pattern fallback) with trampoline
    static const uint8_t kPatFrame[] = { 0x53, 0x48, 0x83, 0xec, 0x60 };
    uint8_t* pFrameTarget = resolve_rva_or_pattern(pBase, 0x3a48b00, kPatFrame, "xxxxx", sizeof(kPatFrame));
    if (pFrameTarget && pFrameTarget[0] == 0x53 && pFrameTarget[1] == 0x48 && pFrameTarget[2] == 0x83 && pFrameTarget[3] == 0xec && pFrameTarget[4] == 0x60) {
        memcpy(g_tramp_frame_buf, pFrameTarget, 15);
        g_tramp_frame_buf[15] = 0xFF;
        g_tramp_frame_buf[16] = 0x25;
        g_tramp_frame_buf[17] = 0x00;
        g_tramp_frame_buf[18] = 0x00;
        g_tramp_frame_buf[19] = 0x00;
        g_tramp_frame_buf[20] = 0x00;
        *(uint64_t*)(g_tramp_frame_buf + 21) = (uint64_t)(uintptr_t)(pFrameTarget + 15);

        DWORD oldProtect;
        VirtualProtect(g_tramp_frame_buf, sizeof(g_tramp_frame_buf), PAGE_EXECUTE_READWRITE, &oldProtect);
        g_trampoline_launcher_model_frame = (pfn_launcher_model_frame)(uintptr_t)g_tramp_frame_buf;

        VirtualProtect(pFrameTarget, 15, PAGE_EXECUTE_READWRITE, &oldProtect);
        pFrameTarget[0] = 0xFF;
        pFrameTarget[1] = 0x25;
        pFrameTarget[2] = 0x00;
        pFrameTarget[3] = 0x00;
        pFrameTarget[4] = 0x00;
        pFrameTarget[5] = 0x00;
        *(uint64_t*)(pFrameTarget + 6) = (uint64_t)(uintptr_t)Hooked_launcher_model_frame;
        pFrameTarget[14] = 0x90;
        VirtualProtect(pFrameTarget, 15, oldProtect, &oldProtect);
        log_hotkey("[LAUNCHER] Hooked launcher_model_frame successfully with trampoline");
    }
}

static int Hooked_SDL_PollEvent(void* ev_ptr);
static int Hooked_SDL_WaitEvent(void* ev_ptr);
static int Hooked_SDL_WaitEventTimeout(void* ev_ptr, int timeout);

static int fetch_next_event(void* ev_ptr, int call_type, int timeout) {
    if (call_type == 1) return Hooked_SDL_WaitEvent(ev_ptr);
    if (call_type == 2) return Hooked_SDL_WaitEventTimeout(ev_ptr, timeout);
    return Hooked_SDL_PollEvent(ev_ptr);
}

static int s_back_held = 0;
static int s_start_held = 0;

static int handle_sdl_event_interception(void* ev_ptr, int call_type, int timeout) {
    if (!ev_ptr) return 0;

    SDL_Event_t* event = (SDL_Event_t*)ev_ptr;

    // Check if launcher is currently capturing a binding
    int is_capturing = 0;
    int is_pad_capturing = 0;
    if (g_active_launcher_model) {
        uint8_t* pm = (uint8_t*)g_active_launcher_model;
        if (pm[0xc1bd] == 1) {
            is_capturing = 1;
        }
        if (pm[0xc1a8] == 1) {
            is_capturing = 1;
            is_pad_capturing = 1;
        }
    }

    if (is_capturing) {
        // Allow Escape key to cancel capture mode cleanly
        if (event->type == 0x300 && event->key.keysym.sym == 27) {
            uint8_t* pm = (uint8_t*)g_active_launcher_model;
            pm[0xc1a8] = 0;
            pm[0xc1b4] = 0;
            pm[0xc1bd] = 0;
            g_active_capture_btn = -1;
            return fetch_next_event(ev_ptr, call_type, timeout);
        }

        // Drop button releases while in capture mode
        if (event->type == 0x652 || event->type == 0x604) {
            return 0;
        }

        int is_pad_press = 0;
        uint8_t pad_btn = 255;
        int is_trigger = 0;
        int trigger_axis = 0;

        if (event->type == 0x651 || event->type == 0x603) {
            is_pad_press = 1;
            pad_btn = *((uint8_t*)ev_ptr + 12);
        } else if (event->type == 0x650) {
            uint8_t axis = *((uint8_t*)ev_ptr + 12);
            int16_t value = *((int16_t*)((uint8_t*)ev_ptr + 16));
            if ((axis == 4 || axis == 5) && value > 16000) {
                is_pad_press = 1;
                is_trigger = 1;
                trigger_axis = axis;
            } else {
                return 0;
            }
        }

        if (is_pad_press) {
            uint8_t* pm = (uint8_t*)g_active_launcher_model;
            int btn_idx = g_active_capture_btn;
            if (btn_idx < 0 || btn_idx > 11) {
                if (is_pad_capturing) {
                    btn_idx = (int)*(uint32_t*)(pm + 0xc1ac);
                } else {
                    btn_idx = (int)*(uint32_t*)(pm + 0xc1c0);
                }
            }
            if (btn_idx < 0 || btn_idx > 11) btn_idx = 0;

            uint8_t gba_action = s_capture_to_gba[btn_idx];

            if (is_trigger) {
                if (trigger_axis == 4) {
                    g_trigger_l_target = gba_action;
                } else {
                    g_trigger_r_target = gba_action;
                }
            } else {
                if (pad_btn < 15) {
                    g_btn_map[pad_btn] = gba_action;
                }
            }

            save_gamepad_config();
            sync_launcher_gamepad_strings(g_active_launcher_model);

            // Clear capture flags
            pm[0xc1a8] = 0;
            pm[0xc1b4] = 0;
            pm[0xc1bd] = 0;
            g_active_capture_btn = -1;

            char log_buf[128];
            snprintf(log_buf, sizeof(log_buf),
                "[LAUNCHER] Controller captured row=%d -> GBA action=%d", btn_idx, gba_action);
            log_hotkey(log_buf);

            // If keyboard capture was active, synthesize keypress so keyboard column accepts it
            if (!is_pad_capturing) {
                int32_t scancode = 4;
                int32_t sym = 'a';
                if (!is_trigger) {
                    switch (pad_btn) {
                        case 0:  scancode = 4;   sym = 'a'; break;
                        case 1:  scancode = 5;   sym = 'b'; break;
                        case 2:  scancode = 27;  sym = 'x'; break;
                        case 3:  scancode = 28;  sym = 'y'; break;
                        case 4:  scancode = 229; sym = 1073742053; break;
                        case 6:  scancode = 40;  sym = '\r'; break;
                        case 9:  scancode = 15;  sym = 'l'; break;
                        case 10: scancode = 21;  sym = 'r'; break;
                        default: break;
                    }
                }
                memset(ev_ptr, 0, 56);
                *(uint32_t*)((uint8_t*)ev_ptr + 0) = 0x300; // SDL_KEYDOWN
                *(uint32_t*)((uint8_t*)ev_ptr + 4) = GetTickCount();
                *((uint8_t*)ev_ptr + 12) = 1;
                *((uint8_t*)ev_ptr + 13) = 0;
                *(int32_t*)((uint8_t*)ev_ptr + 16) = scancode;
                *(int32_t*)((uint8_t*)ev_ptr + 20) = sym;
                return 1;
            }

            // Drop event so launcher finishes capture cleanly
            return fetch_next_event(ev_ptr, call_type, timeout);
        }
    }

    load_gamepad_config();

    // Check in-game controller hotkeys: Xbox Guide (5) or Back+Start (4+6)
    if (event->type == 0x651) { // SDL_CONTROLLERBUTTONDOWN
        uint8_t pBtn = *((uint8_t*)ev_ptr + 12);
        if (pBtn == 4) s_back_held = 1;
        if (pBtn == 6) s_start_held = 1;

        if (pBtn == 5 || (s_back_held && pBtn == 6) || (s_start_held && pBtn == 4)) { // Guide OR Back+Start
            apply_preset(g_current_preset + 1);
            return fetch_next_event(ev_ptr, call_type, timeout);
        }
    } else if (event->type == 0x652) { // SDL_CONTROLLERBUTTONUP
        uint8_t pBtn = *((uint8_t*)ev_ptr + 12);
        if (pBtn == 4) s_back_held = 0;
        if (pBtn == 6) s_start_held = 0;
    }

    // Gamepad button remapping: SDL_CONTROLLERBUTTONDOWN (0x651) or SDL_CONTROLLERBUTTONUP (0x652)
    if (event->type == 0x651 || event->type == 0x652) {
        uint8_t* pBtn = ((uint8_t*)ev_ptr) + 12;
        if (*pBtn < 15) {
            uint8_t target = g_btn_map[*pBtn];
            if (target == 255) {
                return fetch_next_event(ev_ptr, call_type, timeout);
            }
            *pBtn = target;
        }
    }
    // Generic joystick buttons: SDL_JOYBUTTONDOWN (0x603) or SDL_JOYBUTTONUP (0x604)
    else if (event->type == 0x603 || event->type == 0x604) {
        uint8_t* pBtn = ((uint8_t*)ev_ptr) + 12;
        if (*pBtn < 15) {
            uint8_t target = g_btn_map[*pBtn];
            if (target == 255) {
                return fetch_next_event(ev_ptr, call_type, timeout);
            }
            *pBtn = target;
        }
    }
    // Analog trigger translation: SDL_CONTROLLERAXISMOTION (0x650)
    else if (event->type == 0x650) {
        uint8_t axis = *((uint8_t*)ev_ptr + 12);
        int16_t value = *((int16_t*)((uint8_t*)ev_ptr + 16));

        if (axis == 4 && g_trigger_l_target >= 0) { // Left Trigger (LT)
            if (value > 16000 && !g_lt_active) {
                g_lt_active = 1;
                *(uint32_t*)ev_ptr = 0x651; // SDL_CONTROLLERBUTTONDOWN
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_l_target;
                *((uint8_t*)ev_ptr + 13) = 1; // SDL_PRESSED
            } else if (value < 8000 && g_lt_active) {
                g_lt_active = 0;
                *(uint32_t*)ev_ptr = 0x652; // SDL_CONTROLLERBUTTONUP
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_l_target;
                *((uint8_t*)ev_ptr + 13) = 0; // SDL_RELEASED
            }
        }
        else if (axis == 5 && g_trigger_r_target >= 0) { // Right Trigger (RT)
            if (value > 16000 && !g_rt_active) {
                g_rt_active = 1;
                *(uint32_t*)ev_ptr = 0x651; // SDL_CONTROLLERBUTTONDOWN
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_r_target;
                *((uint8_t*)ev_ptr + 13) = 1; // SDL_PRESSED
            } else if (value < 8000 && g_rt_active) {
                g_rt_active = 0;
                *(uint32_t*)ev_ptr = 0x652; // SDL_CONTROLLERBUTTONUP
                *((uint8_t*)ev_ptr + 12) = (uint8_t)g_trigger_r_target;
                *((uint8_t*)ev_ptr + 13) = 0; // SDL_RELEASED
            }
        }
    }
    else if (event->type == 0x300 || event->type == 0x301) { // SDL_KEYDOWN or SDL_KEYUP
        uint32_t sym = (uint32_t)event->key.keysym.sym;
        uint32_t scancode = event->key.keysym.scancode;

        // F9: Quick Controller Preset Switcher
        if (sym == 0x40000042 || scancode == 66) {
            if (event->type == 0x300 && !event->key.repeat) {
                apply_preset(g_current_preset + 1);
            }
            return fetch_next_event(ev_ptr, call_type, timeout);
        }

        // F10: Toggle decorative bezel border on/off (Clean Fullscreen mode)
        if (sym == 0x40000043 || scancode == 67) {
            if (event->type == 0x300) {
                if (event->key.repeat == 0) {
                    load_bezel_config();
                    g_bezel_enabled = !g_bezel_enabled;
                    snprintf(g_toast_msg, sizeof(g_toast_msg), g_bezel_enabled ? "BEZEL: Custom Border Active (F10 to toggle)" : "BEZEL: Clean Fullscreen (No Borders)");
                    g_toast_expiry = GetTickCount() + 3000;
                    log_hotkey(g_bezel_enabled ? "[HOTKEY] F10 pressed -> Bezel Borders Enabled" : "[HOTKEY] F10 pressed -> Clean Fullscreen (No Borders) Enabled");
                }
            }
            return fetch_next_event(ev_ptr, call_type, timeout);
        }
        // F11: Open/toggle overlay menu (replaces Escape)
        if (sym == 0x40000044 || scancode == 68) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return fetch_next_event(ev_ptr, call_type, timeout);
                }
                log_hotkey("[HOTKEY] F11 pressed -> translated to Escape (overlay menu toggle)");
            }
            event->key.keysym.sym = 27;      // SDLK_ESCAPE
            event->key.keysym.scancode = 41;  // SDL_SCANCODE_ESCAPE
            event->key.keysym.mod = 0;
        }
        // Esc: Exit the game cleanly
        else if (sym == 27 || scancode == 41) {
            if (event->type == 0x300) {
                log_hotkey("[HOTKEY] Escape pressed -> translated to SDL_QUIT (clean exit)");
                event->type = 0x100; // SDL_QUIT
            } else {
                // Drop KEYUP for Esc
                return fetch_next_event(ev_ptr, call_type, timeout);
            }
        }
        // F1: Save state (Slot 1)
        else if (sym == 0x4000003a || scancode == 58) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return fetch_next_event(ev_ptr, call_type, timeout);
                }
                log_hotkey("[HOTKEY] F1 pressed -> forced Shift (save state slot 1)");
            }
            event->key.keysym.sym = 0x4000003a; // SDLK_F1
            event->key.keysym.scancode = 58;    // SDL_SCANCODE_F1
            event->key.keysym.mod |= 1;         // KMOD_LSHIFT -> triggers Save State
        }
        // F2: Load state (Slot 1)
        else if (sym == 0x4000003b || scancode == 59) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return fetch_next_event(ev_ptr, call_type, timeout);
                }
                log_hotkey("[HOTKEY] F2 pressed -> translated to F1 without Shift (load state slot 1)");
            }
            event->key.keysym.sym = 0x4000003a; // SDLK_F1
            event->key.keysym.scancode = 58;    // SDL_SCANCODE_F1
            event->key.keysym.mod = 0;          // Clear Shift -> triggers Load State
        }
    }

    return 1;
}

static int Hooked_SDL_PollEvent(void* ev_ptr) {
    if (!g_real_PollEvent) return 0;
    int res = g_real_PollEvent(ev_ptr);
    if (!res || !ev_ptr) return res;
    return handle_sdl_event_interception(ev_ptr, 0, 0);
}

static int Hooked_SDL_WaitEvent(void* ev_ptr) {
    if (!g_real_WaitEvent) return 0;
    int res = g_real_WaitEvent(ev_ptr);
    if (!res || !ev_ptr) return res;
    return handle_sdl_event_interception(ev_ptr, 1, 0);
}

static int Hooked_SDL_WaitEventTimeout(void* ev_ptr, int timeout) {
    if (!g_real_WaitEventTimeout) return 0;
    int res = g_real_WaitEventTimeout(ev_ptr, timeout);
    if (!res || !ev_ptr) return res;
    return handle_sdl_event_interception(ev_ptr, 2, timeout);
}

static void save_bmp32(const char* filename, int w, int h, const uint8_t* rgba_pixels) {
    FILE* f = fopen(filename, "wb");
    if (!f) return;
    
    uint32_t file_size = 54 + w * h * 4;
    uint8_t header[54] = {
        'B', 'M',
        file_size & 0xFF, (file_size >> 8) & 0xFF, (file_size >> 16) & 0xFF, (file_size >> 24) & 0xFF,
        0, 0, 0, 0,
        54, 0, 0, 0,
        40, 0, 0, 0,
        w & 0xFF, (w >> 8) & 0xFF, (w >> 16) & 0xFF, (w >> 24) & 0xFF,
        h & 0xFF, (h >> 8) & 0xFF, (h >> 16) & 0xFF, (h >> 24) & 0xFF,
        1, 0,
        32, 0,
        0, 0, 0, 0,
        (w * h * 4) & 0xFF, ((w * h * 4) >> 8) & 0xFF, ((w * h * 4) >> 16) & 0xFF, ((w * h * 4) >> 24) & 0xFF,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    fwrite(header, 1, 54, f);
    
    uint8_t* row_buf = (uint8_t*)malloc(w * 4);
    if (row_buf) {
        for (int y = h - 1; y >= 0; --y) {
            const uint8_t* src_row = rgba_pixels + y * w * 4;
            for (int x = 0; x < w; ++x) {
                row_buf[x * 4 + 0] = src_row[x * 4 + 2]; // B
                row_buf[x * 4 + 1] = src_row[x * 4 + 1]; // G
                row_buf[x * 4 + 2] = src_row[x * 4 + 0]; // R
                row_buf[x * 4 + 3] = src_row[x * 4 + 3]; // A
            }
            fwrite(row_buf, 1, w * 4, f);
        }
        free(row_buf);
    }
    fclose(f);
}

static void load_bezel_pixels(void) {
    if (g_bezel_pixels) return;
    
    char exe_dir[MAX_PATH];
    exe_dir[0] = '\0';
    GetModuleFileNameA(NULL, exe_dir, MAX_PATH);
    char* p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';
    
    const char* candidate_dirs[] = {
        exe_dir,
        "assets\\img",
        "."
    };
    
    char path[MAX_PATH];
    for (size_t i = 0; i < sizeof(candidate_dirs)/sizeof(candidate_dirs[0]); ++i) {
        snprintf(path, MAX_PATH, "%s\\assets\\img\\bezel_master_1080p.raw", candidate_dirs[i]);
        FILE* f = fopen(path, "rb");
        if (!f) {
            snprintf(path, MAX_PATH, "%s\\bezel_master_1080p.raw", candidate_dirs[i]);
            f = fopen(path, "rb");
        }
        if (f) {
            g_bezel_pixels = (uint8_t*)malloc(8294400);
            if (g_bezel_pixels) {
                fread(g_bezel_pixels, 1, 8294400, f);
            }
            fclose(f);
            break;
        }
    }
}

static int Hooked_SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect) {
    load_bezel_config();
    if (g_bezel_enabled && texture != g_tex_bezel && texture != g_tex_toast) {
        int sw = 0, sh = 0;
        if (g_real_GetRendererOutputSize) g_real_GetRendererOutputSize(renderer, &sw, &sh);
        if (sw > 0 && sh > 0) {
            // Cutout in 1920x1080 master: [289, 164, 1336, 761]
            SDL_Rect game_rect;
            game_rect.x = (sw * 289) / 1920;
            game_rect.y = (sh * 164) / 1080;
            game_rect.w = (sw * 1336) / 1920;
            game_rect.h = (sh * 761) / 1080;
            return g_real_RenderCopy(renderer, texture, srcrect, &game_rect);
        }
    }
    return g_real_RenderCopy(renderer, texture, srcrect, dstrect);
}

void Hooked_SDL_RenderPresent(SDL_Renderer* renderer) {
    load_bezel_config();
    if (renderer != g_last_renderer) {
        if (g_tex_bezel && g_real_DestroyTexture) g_real_DestroyTexture(g_tex_bezel);
        g_tex_bezel = NULL;
        if (g_tex_toast && g_real_DestroyTexture) g_real_DestroyTexture(g_tex_toast);
        g_tex_toast = NULL;
        g_last_renderer = renderer;
    }
    
    if (g_bezel_enabled) {
        if (!g_tex_bezel && g_real_CreateTexture && g_real_UpdateTexture) {
            load_bezel_pixels();
            if (g_bezel_pixels) {
                // SDL_PIXELFORMAT_ABGR8888 (0x16762004): Byte 0 = R, Byte 1 = G, Byte 2 = B, Byte 3 = A
                g_tex_bezel = g_real_CreateTexture(renderer, 0x16762004, 0, 1920, 1080);
                if (g_tex_bezel) {
                    if (g_real_SetTextureBlendMode) {
                        g_real_SetTextureBlendMode(g_tex_bezel, 1);
                    }
                    g_real_UpdateTexture(g_tex_bezel, NULL, g_bezel_pixels, 1920 * 4);
                }
            }
        }
        
        if (g_real_GetRendererOutputSize && g_real_RenderCopy) {
            int screen_w = 0, screen_h = 0;
            g_real_GetRendererOutputSize(renderer, &screen_w, &screen_h);
            
            if (screen_w > 0 && screen_h > 0) {
                if (g_real_RenderSetViewport) g_real_RenderSetViewport(renderer, NULL);
                if (g_real_RenderSetClipRect) g_real_RenderSetClipRect(renderer, NULL);
                
                if (g_tex_bezel) {
                    g_real_RenderCopy(renderer, g_tex_bezel, NULL, NULL);
                }
                
                if (getenv("MOTHER3_DEBUG_SNAPSHOT")) {
                    g_present_count++;
                    if (g_present_count == 120 && g_real_RenderReadPixels) {
                        char snap_path[MAX_PATH];
                        snprintf(snap_path, MAX_PATH, "bezel_new_live.bmp");
                        uint8_t* cap_buf = (uint8_t*)malloc(screen_w * screen_h * 4);
                        if (cap_buf) {
                            if (g_real_RenderReadPixels(renderer, NULL, 0x16762004, cap_buf, screen_w * 4) == 0) {
                                save_bmp32(snap_path, screen_w, screen_h, cap_buf);
                            }
                            free(cap_buf);
                        }
                    }
                }
            }
        }
    }

    // On-screen Toast notification
    if (GetTickCount() < g_toast_expiry && g_real_GetRendererOutputSize && g_real_RenderCopy) {
        if (!g_tex_toast && g_real_CreateTexture) {
            g_tex_toast = g_real_CreateTexture(renderer, 0x16762004, 0, TOAST_W, TOAST_H);
            if (g_tex_toast && g_real_SetTextureBlendMode) {
                g_real_SetTextureBlendMode(g_tex_toast, 1);
            }
        }
        if (g_tex_toast && g_real_UpdateTexture) {
            int screen_w = 0, screen_h = 0;
            g_real_GetRendererOutputSize(renderer, &screen_w, &screen_h);
            if (screen_w > 0 && screen_h > 0) {
                draw_toast_text(g_toast_msg);
                g_real_UpdateTexture(g_tex_toast, NULL, g_toast_pixels, TOAST_W * 4);
                SDL_Rect dst_toast;
                dst_toast.x = (screen_w - TOAST_W) / 2;
                dst_toast.y = 25;
                dst_toast.w = TOAST_W;
                dst_toast.h = TOAST_H;
                if (g_real_RenderSetViewport) g_real_RenderSetViewport(renderer, NULL);
                if (g_real_RenderSetClipRect) g_real_RenderSetClipRect(renderer, NULL);
                g_real_RenderCopy(renderer, g_tex_toast, NULL, &dst_toast);
            }
        }
    }
    
    if (g_real_RenderPresent) {
        g_real_RenderPresent(renderer);
    }
}

__declspec(dllexport) int SDL_DYNAPI_entry(uint32_t apiver, void *table, uint32_t tablesize) {
    HMODULE hHost = GetModuleHandleA(NULL);
    if (!hHost) return -1;
    
    pfn_dynapi_entry real_entry = (pfn_dynapi_entry)(uintptr_t)GetProcAddress(hHost, "SDL_DYNAPI_entry");
    if (!real_entry) {
        static const uint8_t kPatDynApi[] = { 0x48, 0x83, 0xec, 0x28, 0x89, 0xc8, 0x48, 0x89, 0xd1, 0x41, 0x81, 0xf8 };
        uint8_t* pDyn = resolve_rva_or_pattern((uint8_t*)hHost, 0x3ad8b10, kPatDynApi, "xxxxxxxxxxxx", sizeof(kPatDynApi));
        real_entry = (pfn_dynapi_entry)(uintptr_t)(pDyn ? pDyn : ((uint8_t*)hHost + 0x3ad8b10));
    }
    int res = real_entry(apiver, table, tablesize);
    if (res < 0) return res;
    
    void** ptable = (void**)table;
    g_real_RenderPresent = (pfn_SDL_RenderPresent)ptable[309];
    g_real_RenderCopy = (pfn_SDL_RenderCopy)ptable[306];
    g_real_CreateTexture = (pfn_SDL_CreateTexture)ptable[269];
    g_real_UpdateTexture = (pfn_SDL_UpdateTexture)ptable[278];
    g_real_GetRendererOutputSize = (pfn_SDL_GetRendererOutputSize)ptable[268];
    g_real_DestroyTexture = (pfn_SDL_DestroyTexture)ptable[310];
    g_real_RenderSetViewport = (pfn_SDL_RenderSetViewport)ptable[285];
    g_real_RenderSetClipRect = (pfn_SDL_RenderSetClipRect)ptable[287];
    g_real_RenderReadPixels = (pfn_SDL_RenderReadPixels)ptable[308];
    g_real_SetTextureBlendMode = (pfn_SDL_SetTextureBlendMode)ptable[276];
    g_real_PollEvent = (pfn_SDL_PollEvent)ptable[85];
    g_real_WaitEvent = (pfn_SDL_WaitEvent)ptable[86];
    g_real_WaitEventTimeout = (pfn_SDL_WaitEventTimeout)ptable[87];
    
    ptable[309] = (void*)Hooked_SDL_RenderPresent;
    ptable[306] = (void*)Hooked_SDL_RenderCopy;
    ptable[85] = (void*)Hooked_SDL_PollEvent;
    ptable[86] = (void*)Hooked_SDL_WaitEvent;
    ptable[87] = (void*)Hooked_SDL_WaitEventTimeout;
    
    log_hotkey("[BEZEL] SDL_DYNAPI_entry initialized and hooked SDL_PollEvent, SDL_WaitEvent, SDL_WaitEventTimeout");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        
        // Dynamically ensure Mother3Recomp.exe accepts both vanilla English v1.3 and HQ Audio mod ROMs
        HMODULE hHost = GetModuleHandleA(NULL);
        if (hHost) {
            uint8_t* pBase = (uint8_t*)hHost;
            uint8_t* pCode = pBase + 0x37ca1e3;
            if (pCode[0] == 0x75 && pCode[1] == 0x8b) {
                DWORD oldProtect;
                if (VirtualProtect(pCode, 2, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    pCode[0] = 0x90; // nop
                    pCode[1] = 0x90; // nop
                    VirtualProtect(pCode, 2, oldProtect, &oldProtect);
                }
            }

            // Install all launcher controls and capture hooks
            install_launcher_hooks(pBase);
        }
    }
    return TRUE;
}
