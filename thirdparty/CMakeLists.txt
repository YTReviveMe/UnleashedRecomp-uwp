cmake_policy(SET CMP0077 NEW)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)
set(MSDFGEN_DISABLE_PNG ON)

set(SDL2MIXER_DEPS_SHARED OFF)
set(SDL2MIXER_VENDORED ON)
set(SDL2MIXER_FLAC OFF)
set(SDL2MIXER_MOD OFF)
set(SDL2MIXER_MP3 OFF)
set(SDL2MIXER_MIDI OFF)
set(SDL2MIXER_OPUS OFF)
set(SDL2MIXER_VORBIS "VORBISFILE")
set(SDL2MIXER_WAVPACK OFF)


add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/msdf-atlas-gen")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/nativefiledialog-extended")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/o1heap")
# DLW: Linking against prebuilt shared UWP lib, replaced later by UWP wrapper
# > Using throwaway prebuilts because build doesn't like mixing shared/static
set(SDL2_LIBRARY ${CMAKE_SOURCE_DIR}/libuwp/junk/SDL2.lib)
set(SDL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/SDL/include)
#add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/SDL")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/SDL_mixer")

