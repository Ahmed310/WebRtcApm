# meson.build — builds the "WebRtcApm" plugin (apm_c_api shim) against an installed
# webrtc-audio-processing. Used by CI (the GitHub Actions workflow) because meson resolves the static
# dependency graph (abseil / pffft / system libs from the .pc Libs.private) natively and correctly on
# MSVC, clang and the NDK/iOS cross toolchains — which CMake's pkg-config-static handling does not.
#
#   PKG_CONFIG_PATH must point at the webrtc-audio-processing install prefix's lib/pkgconfig.
#   -Dlibkind=shared  -> WebRtcApm.dll / libWebRtcApm.so / libWebRtcApm.dylib   (desktop + Android)
#   -Dlibkind=static  -> libWebRtcApm.a                                          (iOS; archives merged after)

project('webrtcapm', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++17', 'buildtype=release', 'b_staticpic=true'])

# v2 preferred, fall back to v1. static:true pulls the private deps so the shared lib / merged archive
# is self-contained.
apm = dependency('webrtc-audio-processing-2', required: false, static: true)
if not apm.found()
  apm = dependency('webrtc-audio-processing-1', required: true, static: true)
endif

sources = ['apm_c_api.cpp']

if get_option('libkind') == 'static'
  # iOS: Unity links a static library into the app at Xcode-build time.
  static_library('WebRtcApm', sources, dependencies: apm, install: false)
else
  shared_library('WebRtcApm', sources,
    dependencies: apm,
    install: false,
    gnu_symbol_visibility: 'hidden')
endif
