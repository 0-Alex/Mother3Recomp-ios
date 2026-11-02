# FetchContent PATCH_COMMAND for SDL2 (runs in its source dir). Idempotent;
# fails loudly if SDL's source no longer matches.
#
# 1. Core Audio queues enough buffers for at least 2 x 15 ms; the iOS build
#    opens a ~10 ms quantum (host_window.cpp), and with the 15 ms floor SDL
#    would queue four of them (~40 ms). An 8 ms floor keeps it at two
#    (~20 ms), trimming audio latency for rhythm combos.
# 2. The Metal renderer reports when each drawable actually reached the
#    screen (MTLDrawable.presentedTime, 0 = dropped) through
#    SDL_M3_PresentedHook, which feeds the performance HUD (m3_perf_hud).
function(m3_patch file marker old new)
    file(READ "${file}" src)
    string(FIND "${src}" "${marker}" done)
    if(NOT done EQUAL -1)
        return()
    endif()
    string(FIND "${src}" "${old}" at)
    if(at EQUAL -1)
        message(FATAL_ERROR "sdl_ios_patch: ${file} no longer contains: ${old}")
    endif()
    string(REPLACE "${old}" "${new}" src "${src}")
    file(WRITE "${file}" "${src}")
endfunction()

m3_patch("src/audio/coreaudio/SDL_coreaudio.m"
    "MINIMUM_AUDIO_BUFFER_TIME_MS = 8.0;"
    "MINIMUM_AUDIO_BUFFER_TIME_MS = 15.0;"
    "MINIMUM_AUDIO_BUFFER_TIME_MS = 8.0;")

m3_patch("src/render/metal/SDL_render_metal.m"
    "SDL_M3_PresentedHook"
    "#import <QuartzCore/CAMetalLayer.h>\n"
    "#import <QuartzCore/CAMetalLayer.h>\n\n/* Mother3Recomp: called with each drawable's presentedTime (0 = dropped). */\nvoid (*SDL_M3_PresentedHook)(double presented_time) = NULL;\n")

m3_patch("src/render/metal/SDL_render_metal.m"
    "addPresentedHandler"
    "        [data.mtlcmdbuffer presentDrawable:data.mtlbackbuffer];\n"
    "        if (SDL_M3_PresentedHook) {\n            void (*hook)(double) = SDL_M3_PresentedHook;\n            [data.mtlbackbuffer addPresentedHandler:^(id<MTLDrawable> drawable) {\n                hook(drawable.presentedTime);\n            }];\n        }\n        [data.mtlcmdbuffer presentDrawable:data.mtlbackbuffer];\n")
