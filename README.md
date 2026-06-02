# WebRtcApm native plugin — build recipe (FnVoice Phase 6)

This folder builds **`WebRtcApm`**, the native plugin FnVoice uses for AEC / AGC / NS. It is a thin
`extern "C"` shim (`apm_c_api.cpp`) linked against **[webrtc-audio-processing]** — the maintained PulseAudio
fork that packages just WebRTC's Audio Processing Module (APM). The managed side
(`VoiceChat/Scripts/Processing/WebRtcApmPreprocessor.cs`) loads it via `[DllImport("WebRtcApm")]`.

> **Where the built binary goes.** Unity loads native plugins by name. Place the output so Unity's importer
> targets the right platform, mirroring the opus layout:
>
> | Platform              | File                                   | Folder (relative to `Plugins/WebRtcApm/`) |
> |-----------------------|----------------------------------------|--------------------------------------------|
> | Windows / Editor x64  | `WebRtcApm.dll`                        | `x86_64/`                                  |
> | Linux x64             | `libWebRtcApm.so`                      | `linux/`                                   |
> | macOS (universal)     | `WebRtcApm.bundle` (or `.dylib`)       | `macOS/`                                   |
> | iOS                   | `libWebRtcApm.a` (static)              | `iOS/`                                      |
> | Android arm64         | `libWebRtcApm.so`                      | `Android/arm64-v8a/`                        |
>
> After copying, set the plugin's platform/CPU in the Unity importer (Editor auto-detects x86_64; verify the
> others — same lesson as the opus natives). **Start with Windows/Editor x64 only** to validate Phase 6.0.

[webrtc-audio-processing]: https://gitlab.freedesktop.org/pulseaudio/webrtc-audio-processing

---

## Build via GitHub Actions (recommended)

`webrtcapm-build.yml` (in this folder) is a ready-to-use workflow — host it in a **separate repo** (like the
Opus-CSharp one). Repo layout:

```
<repo-root>/
  .github/workflows/webrtcapm-build.yml   <- copy of webrtcapm-build.yml
  apm_c_api.h                              <- copy these 2 (root or ANY subfolder)
  apm_c_api.cpp
```

That's all — the two `apm_c_api.*` files can live anywhere in the repo; the workflow finds them and
**generates the meson build files itself** (nothing else to copy).

Push (or **Actions ▸ Build WebRtcApm ▸ Run workflow**). Each job builds `webrtc-audio-processing` from source
(pinned `WAP_REF`, static) with meson, then builds the shim **also with meson** (meson resolves the static
abseil/pffft/system deps natively). Download the **`webrtcapm-all-platforms`** artifact — already arranged as
`Plugins/WebRtcApm/{x86_64,linux,macOS,Android,iOS}/…`; drop it into the Unity project. Builds **Windows / Linux /
macOS-universal / Android (arm64-v8a, armeabi-v7a, x86_64) / iOS (arm64)**.

The `CMakeLists.txt` / `meson.build` here and the sections below are an alternative for building **locally** on desktop.

---

## 0. Prerequisites

- CMake ≥ 3.16, a C++17 compiler.
- **webrtc-audio-processing** dev package (headers + lib). The shim targets the **v1.x / v2.x** API
  (`AudioProcessingBuilder().Create()`, `Config`, float `ProcessStream`/`ProcessReverseStream`).

---

## 1. Get webrtc-audio-processing

Pick whichever matches your platform:

### Windows (recommended: vcpkg)
```powershell
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
# package name may be 'libwebrtc-audio-processing' or 'webrtc-audio-processing' depending on registry version
.\vcpkg\vcpkg install webrtc-audio-processing:x64-windows
# install prefix: .\vcpkg\installed\x64-windows
```

### Linux (distro package or meson build)
```bash
# Debian/Ubuntu:
sudo apt install libwebrtc-audio-processing-dev pkg-config cmake build-essential
# …or build the fork from source with meson:
git clone https://gitlab.freedesktop.org/pulseaudio/webrtc-audio-processing.git
cd webrtc-audio-processing && meson . build --prefix=$PWD/install && ninja -C build install
# install prefix: $PWD/install  (sets up pkg-config under install/lib/pkgconfig)
```

### macOS
```bash
brew install pkg-config meson ninja
git clone https://gitlab.freedesktop.org/pulseaudio/webrtc-audio-processing.git
cd webrtc-audio-processing && meson . build --prefix=$PWD/install && ninja -C build install
```

### Android / iOS
Cross-compile the fork with meson cross-files (NDK toolchain for Android arm64; the iOS toolchain for a
static `libwebrtc-audio-processing.a`). Defer to Phase 6.3 — get Windows working first.

---

## 2. Build the shim (this folder)

If `pkg-config` can see webrtc-audio-processing (Linux/macOS, or MSYS), CMake finds it automatically:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Otherwise point CMake at the install prefix (Windows/vcpkg or a manual build):

```powershell
cmake -S . -B build -DWEBRTC_APM_ROOT="C:/path/to/vcpkg/installed/x64-windows" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

(With vcpkg you can instead pass `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg/scripts/buildsystems/vcpkg.cmake`.)

---

## 3. Install into the Unity plugin folder

Copy the built library to the platform folder from the table above and rename to the expected name, e.g. Windows:

```powershell
copy build\Release\WebRtcApm.dll ..\x86_64\WebRtcApm.dll
```

> **Static-linking note (recommended for Windows/macOS):** so end users don't need
> webrtc-audio-processing installed, link it **statically** into `WebRtcApm` (vcpkg static triplet
> `x64-windows-static`, or a static `.a`/`.lib`). Otherwise ship the webrtc-audio-processing runtime
> alongside `WebRtcApm.dll`.

---

## 4. Validate (Phase 6.0)

1. In Unity, open the **VoiceChatSettings** asset → enable **Enable Audio Processing** (leave AEC off; NS/AGC on).
2. Enter Play mode in a voice scene and start recording.
3. Expect a log: `[Apm] WebRTC APM active — rate …Hz, AEC False, NS True(L2), AGC True.`
   - If the plugin is missing/incompatible you'll instead see a clear `[Apm] WebRtcApm native plugin not found …`
     and FnVoice transparently falls back to no processing (identity) — capture still works.
4. Phase 6.1 then verifies NS/AGC actually clean the signal; AEC (reverse stream) lands in 6.2.

---

## ABI reference (`apm_c_api.h`)

```
fn_apm_handle fn_apm_create(void);
void          fn_apm_destroy(fn_apm_handle);
int           fn_apm_configure(handle, sample_rate_hz, num_channels,
                               enable_aec, enable_ns, enable_agc, ns_level, agc_mode);
int           fn_apm_process_stream(handle, float* frame, int num_samples);          // 10 ms, in place
int           fn_apm_process_reverse_stream(handle, float* frame, int num_samples);  // 10 ms reference
int           fn_apm_set_stream_delay_ms(handle, int delay_ms);
```
All `int` returns are 0 on success. `num_samples` is always `sample_rate_hz / 100` (one 10 ms frame).
