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

static SDL_Texture* g_tex_bezel = NULL;
static SDL_Renderer* g_last_renderer = NULL;
static uint8_t* g_bezel_pixels = NULL;
static int g_present_count = 0;

static void log_hotkey(const char* msg) {
    if (!getenv("MOTHER3_DEBUG_HOTKEY")) return;
    FILE* f = fopen("hotkey.log", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

static int Hooked_SDL_PollEvent(void* ev_ptr) {
    if (!g_real_PollEvent) return 0;
    int res = g_real_PollEvent(ev_ptr);
    if (!res || !ev_ptr) return res;

    SDL_Event_t* event = (SDL_Event_t*)ev_ptr;

    if (event->type == 0x300 || event->type == 0x301) { // SDL_KEYDOWN or SDL_KEYUP
        uint32_t sym = (uint32_t)event->key.keysym.sym;
        uint32_t scancode = event->key.keysym.scancode;

        // F11: Open/toggle overlay menu (replaces Escape)
        if (sym == 0x40000044 || scancode == 68) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return Hooked_SDL_PollEvent(ev_ptr);
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
                return Hooked_SDL_PollEvent(ev_ptr);
            }
        }
        // F1: Save state (Slot 1)
        else if (sym == 0x4000003a || scancode == 58) {
            if (event->type == 0x300) {
                if (event->key.repeat != 0) {
                    return Hooked_SDL_PollEvent(ev_ptr);
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
                    return Hooked_SDL_PollEvent(ev_ptr);
                }
                log_hotkey("[HOTKEY] F2 pressed -> translated to F1 without Shift (load state slot 1)");
            }
            event->key.keysym.sym = 0x4000003a; // SDLK_F1
            event->key.keysym.scancode = 58;    // SDL_SCANCODE_F1
            event->key.keysym.mod = 0;          // Clear Shift -> triggers Load State
        }
    }

    return res;
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
    if (texture != g_tex_bezel) {
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
    if (renderer != g_last_renderer) {
        if (g_tex_bezel && g_real_DestroyTexture) g_real_DestroyTexture(g_tex_bezel);
        g_tex_bezel = NULL;
        g_last_renderer = renderer;
    }
    
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
    
    if (g_real_RenderPresent) {
        g_real_RenderPresent(renderer);
    }
}

__declspec(dllexport) int SDL_DYNAPI_entry(uint32_t apiver, void *table, uint32_t tablesize) {
    HMODULE hHost = GetModuleHandleA(NULL);
    if (!hHost) return -1;
    
    pfn_dynapi_entry real_entry = (pfn_dynapi_entry)((uintptr_t)hHost + 0x3ad8b10);
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
    
    ptable[309] = (void*)Hooked_SDL_RenderPresent;
    ptable[306] = (void*)Hooked_SDL_RenderCopy;
    ptable[85] = (void*)Hooked_SDL_PollEvent;
    
    log_hotkey("[BEZEL] SDL_DYNAPI_entry initialized and hooked SDL_PollEvent");
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
        }
    }
    return TRUE;
}
