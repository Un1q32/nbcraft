cmake_minimum_required(VERSION 3.16.0)
project(nbcraft-vita)

# SDL2 Build
target_compile_definitions(nbcraft-core
    PUBLIC USE_SDL
    PUBLIC USE_GLES
    PUBLIC HANDLE_CHARS_SEPARATELY
    PUBLIC MC_NO_WSTRING
)

# Build
set(SOURCES
    main.cpp
    AppPlatform_vita.cpp
    ../sdl/base/AppPlatform_sdl.cpp
    ../sdl/sdl2/base/AppPlatform_sdl2.cpp
)
add_executable(nbcraft ${SOURCES})

# File logging for debugging on-device (stdout.log in the storage dir)
target_compile_definitions(nbcraft PRIVATE "$<$<CONFIG:DEBUG>:DEBUG_LOG_TO_FILE>")

# Core
target_link_libraries(nbcraft nbcraft-core)

# stb_image
target_link_libraries(nbcraft stb_image)

# SDL2 (from the Vita SDK) + vitaGL + required Vita modules
find_library(SDL2_LIB SDL2 REQUIRED)
find_library(VITAGL_LIB vitaGL REQUIRED)
find_library(VITASHARK_LIB vitashark REQUIRED)
target_link_libraries(nbcraft
    ${SDL2_LIB}
    ${VITAGL_LIB}
    ${VITASHARK_LIB}
    # Needed before pthread, so libstdc++'s pthread dependencies get resolved
    stdc++
    pthread
    mathneon
    SceGxm_stub
    SceDisplay_stub
    SceCtrl_stub
    SceTouch_stub
    SceMotion_stub
    SceHid_stub
    SceIme_stub
    SceAudio_stub
    SceAudioIn_stub
    SceKernelDmacMgr_stub
    SceShaccCg_stub
    SceShaccCgExt
    taihen_stub
    SceSysmodule_stub
    SceAppMgr_stub
    SceCommonDialog_stub
    SceNet_stub
    SceNetCtl_stub
    ScePower_stub
    SceRtc_stub
    m
)

# Audio
# Listed explicitly so the order ends up as: nbcraft-audio -> openal -> pthread/SceAudio,
# resolving openal's pthread/sem/sceAudioOut references (later duplicates from
# platforms/audio are ignored by CMake)
find_library(OPENAL_LIB openal REQUIRED)
target_link_libraries(nbcraft
    nbcraft-audio
    ${OPENAL_LIB}
    pthread
    SceAudio_stub
)

# VPK Packaging
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_TITLE_ID "NBCR00001" CACHE STRING "Vita Title ID")
set(VITA_APP_NAME "NBcraft" CACHE STRING "Vita App Name")
set(VITA_APP_VERSION "01.00" CACHE STRING "Vita App Version")

vita_create_self(
    nbcraft.self
    "${CMAKE_BINARY_DIR}/nbcraft"
    UNSAFE
)
# The macro only tracks the ELF by file path; force proper build order too
add_dependencies(nbcraft-velf nbcraft)

# Pack game/assets into the VPK under assets/ (matches AppPlatform_vita::getAssetPath)
set(VPK_ARGS "")
file(GLOB_RECURSE VITA_ASSET_FILES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/../../game/assets"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../game/assets/*"
)
foreach(asset ${VITA_ASSET_FILES})
    if(asset MATCHES "\\.gitignore$")
        continue()
    endif()
    list(APPEND VPK_ARGS "FILE" "${CMAKE_CURRENT_SOURCE_DIR}/../../game/assets/${asset}" "assets/${asset}")
endforeach()

# LiveArea icons must be 8-bit indexed PNGs, otherwise install fails with 0x8010113D
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(VITA_ICON0 "${CMAKE_CURRENT_BINARY_DIR}/icon0.png")
add_custom_command(OUTPUT "${VITA_ICON0}"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/convert_icon.py" "${CMAKE_CURRENT_SOURCE_DIR}/../../game/assets/app/icons/icon.png" "${VITA_ICON0}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../game/assets/app/icons/icon.png"
    COMMENT "Converting icon0.png to 8-bit palette"
)

vita_create_vpk(
    "${VITA_APP_NAME}-${VITA_APP_VERSION}.vpk"
    "${VITA_TITLE_ID}"
    nbcraft.self
    VERSION "${VITA_APP_VERSION}"
    NAME "${VITA_APP_NAME}"
    ${VPK_ARGS}
    FILE "${VITA_ICON0}" "sce_sys/icon0.png"
)
