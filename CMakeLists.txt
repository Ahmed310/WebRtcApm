# CMakeLists.txt — builds the "WebRtcApm" native plugin: the apm_c_api shim linked against
# webrtc-audio-processing. Output goes next to this file's parent so Unity picks it up per-platform:
#   ../x86_64/WebRtcApm.dll        (Windows / Editor)
#   ../linux/libWebRtcApm.so
#   ../macOS/WebRtcApm.bundle
#   ../Android/arm64-v8a/libWebRtcApm.so
#
# See README.md for the full step-by-step (getting webrtc-audio-processing via vcpkg / pkg-config / source).

cmake_minimum_required(VERSION 3.16)
project(WebRtcApm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(WebRtcApm SHARED apm_c_api.cpp)

# Static-link webrtc-audio-processing into the plugin (one self-contained Unity native lib). The CI
# workflow passes -DWEBRTC_APM_STATIC=ON; for a local dynamic dev build leave it OFF.
option(WEBRTC_APM_STATIC "Link webrtc-audio-processing statically (pull pkg-config Libs.private)" OFF)

# ---- Locate webrtc-audio-processing --------------------------------------------------------
# Option A (Linux/macOS, or Windows with pkgconfiglite): pkg-config. Point PKG_CONFIG_PATH at the
# webrtc-audio-processing install prefix's lib/pkgconfig.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(WEBRTC_APM QUIET webrtc-audio-processing-2)
    if(NOT WEBRTC_APM_FOUND)
        pkg_check_modules(WEBRTC_APM QUIET webrtc-audio-processing-1)
    endif()
endif()

if(WEBRTC_APM_FOUND)
    target_include_directories(WebRtcApm PRIVATE ${WEBRTC_APM_INCLUDE_DIRS})
    if(WEBRTC_APM_STATIC)
        # _STATIC_* pulls Libs.private (abseil, pffft, system libs) needed when static-linking.
        target_link_directories(WebRtcApm PRIVATE ${WEBRTC_APM_STATIC_LIBRARY_DIRS})
        target_link_libraries(WebRtcApm PRIVATE ${WEBRTC_APM_STATIC_LIBRARIES})
    else()
        target_link_directories(WebRtcApm PRIVATE ${WEBRTC_APM_LIBRARY_DIRS})
        target_link_libraries(WebRtcApm PRIVATE ${WEBRTC_APM_LIBRARIES})
    endif()
else()
    # Option B: point CMake at a prebuilt tree (e.g. vcpkg install, or your own build):
    #   cmake -DWEBRTC_APM_ROOT=C:/path/to/webrtc-audio-processing/install ..
    # Expecting: ${WEBRTC_APM_ROOT}/include/<headers>  and  ${WEBRTC_APM_ROOT}/lib/<libs>
    if(NOT DEFINED WEBRTC_APM_ROOT)
        message(FATAL_ERROR
            "webrtc-audio-processing not found. Either install it so pkg-config finds it, "
            "or pass -DWEBRTC_APM_ROOT=<install-prefix> (see README.md).")
    endif()
    target_include_directories(WebRtcApm PRIVATE
        "${WEBRTC_APM_ROOT}/include"
        "${WEBRTC_APM_ROOT}/include/webrtc-audio-processing-2"
        "${WEBRTC_APM_ROOT}/include/webrtc-audio-processing-1")
    find_library(WEBRTC_APM_LIB
        NAMES webrtc-audio-processing-2 webrtc-audio-processing-1 webrtc_audio_processing webrtc-audio-processing
        PATHS "${WEBRTC_APM_ROOT}/lib")
    if(NOT WEBRTC_APM_LIB)
        message(FATAL_ERROR "Could not find the webrtc-audio-processing library under ${WEBRTC_APM_ROOT}/lib")
    endif()
    target_link_libraries(WebRtcApm PRIVATE "${WEBRTC_APM_LIB}")
endif()

# Drop the .dll/.so/.dylib one directory up (into the platform folder you pass), default = ./build output.
set_target_properties(WebRtcApm PROPERTIES
    OUTPUT_NAME "WebRtcApm"
    PREFIX "lib"            # -> libWebRtcApm.so / libWebRtcApm.dylib ; on Windows the import lib only
)

if(WIN32)
    # Windows: produce WebRtcApm.dll (no "lib" prefix) to match [DllImport("WebRtcApm")].
    set_target_properties(WebRtcApm PROPERTIES PREFIX "")
endif()
