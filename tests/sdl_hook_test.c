#define SDL_MAIN_HANDLED
#include <assert.h>
#include <stdio.h>
#include <SDL.h>

static void key(SDL_Scancode scan) {
    SDL_Event e = {0};
    e.type = SDL_KEYDOWN;
    e.key.keysym.scancode = scan;
    e.key.keysym.sym = SDL_GetKeyFromScancode(scan);
    assert(SDL_PushEvent(&e) == 1);
}
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    for (int cycle = 0; cycle < 8; ++cycle) {
        SDL_Window *w = SDL_CreateWindow("hook regression", 0, 0, 960, 640, SDL_WINDOW_HIDDEN);
        assert(w);
        SDL_Renderer *r = SDL_CreateRenderer(w, -1, SDL_RENDERER_SOFTWARE);
        assert(r);
        SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 240, 160);
        assert(t);
        static unsigned char pixels[240 * 160 * 3];
        assert(SDL_UpdateTexture(t, NULL, pixels, 240 * 3) == 0);
        SDL_RenderSetLogicalSize(r, 240, 160);
        assert(SDL_RenderCopy(r, t, NULL, NULL) == 0);
        SDL_RenderPresent(r);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {}
        key(SDL_SCANCODE_F10);
        key(SDL_SCANCODE_F11);
        key(SDL_SCANCODE_ESCAPE);
        int menu = 0, escape = 0;
        while (SDL_PollEvent(&event)) {
            assert(event.type != SDL_QUIT);
            if (event.type != SDL_KEYDOWN) continue;
            assert(event.key.keysym.scancode != SDL_SCANCODE_F10);
            menu += event.key.keysym.scancode == SDL_SCANCODE_F11;
            escape += event.key.keysym.scancode == SDL_SCANCODE_ESCAPE;
        }
        assert(menu == 1 && escape == 1);
        SDL_DestroyTexture(t);
        SDL_DestroyRenderer(r);
        SDL_DestroyWindow(w);
    }
    SDL_Quit();
    puts("SDL hook: F10 consumed, F11/Escape preserved; renderer recreation passed");
    return 0;
}
