# iOS support for Mother3Recomp. Included from CMakeLists.txt when IOS is set
# (configure with -DCMAKE_SYSTEM_NAME=iOS). Builds SDL2 from source as a static
# library and exposes it as SDL2::SDL2, the same target the Android shell gives
# gbarecomp, so gbarecomp's existing "parent-provided SDL2" path is reused.
enable_language(OBJCXX)
include(FetchContent)

# SDL2's own CMake files declare an old minimum version; CMake 4 rejects that
# unless told otherwise. Ignored by older CMake versions.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(SDL2)

if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED GLOBAL)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_LINK_LIBRARIES SDL2-static
        INTERFACE_INCLUDE_DIRECTORIES "${sdl2_SOURCE_DIR}/include")
endif()

# No GL launcher / ImGui runtime UI on iOS yet (they are desktop GL or Android
# GLES only); the game runs headless-of-UI like the Android --no-launcher path.
set(GBARECOMP_RUNTIME_UI_ROOT "" CACHE PATH "" FORCE)
set(GBAGAME_RECOMP_UI OFF CACHE BOOL "" FORCE)

# Frameworks SDL2 and the game need on iOS. Listed explicitly so the link does
# not depend on SDL's static-library dependency propagation.
set(M3_IOS_LINK_LIBS
    "-framework UIKit" "-framework Foundation" "-framework AudioToolbox"
    "-framework AVFoundation" "-framework CoreAudio" "-framework CoreGraphics"
    "-framework CoreHaptics" "-framework CoreMotion" "-framework CoreBluetooth"
    "-framework GameController" "-framework Metal" "-framework OpenGLES"
    "-framework QuartzCore" "-framework CoreVideo" "-framework ImageIO"
    "-framework CoreFoundation" "-liconv")
