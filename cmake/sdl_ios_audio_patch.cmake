# FetchContent PATCH_COMMAND for SDL2 (run in its source dir). SDL's Core
# Audio backend queues enough buffers for at least 2 x 15 ms; the iOS build
# opens a ~10 ms quantum (host_window.cpp), and with the 15 ms floor SDL
# would queue four of them (~40 ms). An 8 ms floor keeps it at two (~20 ms),
# which trims audio latency for rhythm combos. Idempotent.
set(_f "src/audio/coreaudio/SDL_coreaudio.m")
file(READ "${_f}" _src)
string(FIND "${_src}" "MINIMUM_AUDIO_BUFFER_TIME_MS = 8.0;" _done)
if(_done EQUAL -1)
    string(FIND "${_src}" "MINIMUM_AUDIO_BUFFER_TIME_MS = 15.0;" _at)
    if(_at EQUAL -1)
        message(FATAL_ERROR "sdl_ios_audio_patch: SDL_coreaudio.m no longer has the 15 ms floor")
    endif()
    string(REPLACE "MINIMUM_AUDIO_BUFFER_TIME_MS = 15.0;" "MINIMUM_AUDIO_BUFFER_TIME_MS = 8.0;" _src "${_src}")
    file(WRITE "${_f}" "${_src}")
endif()
