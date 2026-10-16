#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>

/* Use the pinned SDL declarations so table offsets and event layouts agree. */
typedef struct {
#define SDL_DYNAPI_PROC(rc, fn, params, args, ret) rc (SDLCALL *fn) params;
#include "SDL_dynapi_procs.h"
#undef SDL_DYNAPI_PROC
} SdlApi;
static SdlApi api;
static SDL_Texture *game_texture, *bezel_texture;
static SDL_Renderer *bezel_renderer;
static int bezel_enabled = 1;

static SDL_Texture *SDLCALL create_texture(SDL_Renderer *r, Uint32 f, int a, int w, int h) {
    SDL_Texture *t = api.SDL_CreateTexture(r, f, a, w, h);
    if (a == SDL_TEXTUREACCESS_STREAMING && w == 240 && h == 160) game_texture = t;
    return t;
}
static void SDLCALL destroy_texture(SDL_Texture *t) {
    if (t == game_texture) game_texture = NULL;
    if (t == bezel_texture) bezel_texture = NULL;
    api.SDL_DestroyTexture(t);
}
static void SDLCALL destroy_renderer(SDL_Renderer *r) {
    if (r == bezel_renderer) {
        bezel_texture = NULL;
        bezel_renderer = NULL;
    }
    game_texture = NULL;
    api.SDL_DestroyRenderer(r);
}
static int SDLCALL poll_event(SDL_Event *e) {
    int result;
    while ((result = api.SDL_PollEvent(e)) && e) {
        if ((e->type == SDL_KEYDOWN || e->type == SDL_KEYUP) &&
            e->key.keysym.scancode == SDL_SCANCODE_F10) {
            if (e->type == SDL_KEYDOWN && !e->key.repeat) bezel_enabled = !bezel_enabled;
            continue;
        }
        break;
    }
    return result;
}
static int load_bezel(SDL_Renderer *r) {
    char path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, path, sizeof(path))) return 0;
    char *slash = strrchr(path, '\\');
    if (!slash) return 0;
    *slash = 0;
    if (strlen(path) + sizeof("\\assets\\img\\bezel_master_1080p.raw") > sizeof(path)) return 0;
    strcat(path, "\\assets\\img\\bezel_master_1080p.raw");
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    const size_t bytes = 1920u * 1080u * 4u;
    void *pixels = HeapAlloc(GetProcessHeap(), 0, bytes);
    int valid = pixels && fread(pixels, 1, bytes, file) == bytes;
    fclose(file);
    if (valid) {
        bezel_texture = api.SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888,
                                              SDL_TEXTUREACCESS_STATIC, 1920, 1080);
        if (bezel_texture) {
            api.SDL_SetTextureBlendMode(bezel_texture, SDL_BLENDMODE_BLEND);
            api.SDL_UpdateTexture(bezel_texture, NULL, pixels, 1920 * 4);
            bezel_renderer = r;
        }
    }
    if (pixels) HeapFree(GetProcessHeap(), 0, pixels);
    return bezel_texture != NULL;
}
static int SDLCALL render_copy(SDL_Renderer *r, SDL_Texture *t,
                              const SDL_Rect *src, const SDL_Rect *dst) {
    if (!bezel_enabled || t != game_texture ||
        (!bezel_texture && !load_bezel(r))) return api.SDL_RenderCopy(r, t, src, dst);
    int w, h, lw, lh;
    if (api.SDL_GetRendererOutputSize(r, &w, &h) != 0 || w <= 0 || h <= 0)
        return api.SDL_RenderCopy(r, t, src, dst);
    SDL_Rect viewport, clip;
    SDL_bool clipped = api.SDL_RenderIsClipEnabled(r);
    api.SDL_RenderGetLogicalSize(r, &lw, &lh);
    api.SDL_RenderGetViewport(r, &viewport);
    api.SDL_RenderGetClipRect(r, &clip);
    api.SDL_RenderSetLogicalSize(r, 0, 0);
    api.SDL_RenderSetViewport(r, NULL);
    api.SDL_RenderSetClipRect(r, NULL);
    SDL_Rect game = {w * 289 / 1920, h * 164 / 1080, w * 1336 / 1920, h * 761 / 1080};
    int result = api.SDL_RenderCopy(r, t, src, &game);
    /* Draw beneath the runtime menu, never over it at RenderPresent. */
    api.SDL_RenderCopy(r, bezel_texture, NULL, NULL);
    api.SDL_RenderSetLogicalSize(r, lw, lh);
    api.SDL_RenderSetViewport(r, &viewport);
    api.SDL_RenderSetClipRect(r, clipped ? &clip : NULL);
    return result;
}
__declspec(dllexport) Sint32 SDLCALL SDL_DYNAPI_entry(Uint32 version, void *table, Uint32 size) {
    typedef Sint32 (SDLCALL *Entry)(Uint32, void *, Uint32);
    HMODULE sdl = GetModuleHandleA("SDL2.dll");
    Entry entry = sdl ? (Entry)GetProcAddress(sdl, "SDL_DYNAPI_entry") : NULL;
    if (!entry || size < offsetof(SdlApi, SDL_RenderIsClipEnabled) + sizeof(void *)) return -1;
    int result = entry(version, table, size);
    if (result < 0) return result;
    memcpy(&api, table, size < sizeof(api) ? size : sizeof(api));
    SdlApi *hooks = (SdlApi *)table;
    hooks->SDL_CreateTexture = create_texture;
    hooks->SDL_DestroyTexture = destroy_texture;
    hooks->SDL_DestroyRenderer = destroy_renderer;
    hooks->SDL_RenderCopy = render_copy;
    hooks->SDL_PollEvent = poll_event;
    return 0;
}
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(instance);
    return TRUE;
}
