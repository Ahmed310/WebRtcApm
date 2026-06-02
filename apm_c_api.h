/*
 * apm_c_api.h — flat C ABI over the WebRTC Audio Processing Module (webrtc-audio-processing).
 *
 * webrtc-audio-processing is a C++ library with no stable C ABI, and P/Invoke cannot call mangled C++
 * methods. This shim exposes a tiny extern-"C" surface that the Unity binding (WebRtcApmPreprocessor.cs)
 * imports as the native plugin "WebRtcApm".
 *
 * Audio convention: deinterleaved 32-bit float, range [-1, 1]; the APM works strictly on 10 ms frames at
 * the configured stream sample rate (num_samples == sample_rate_hz / 100). The binding does the chunking.
 *
 * All functions are no-throw across the boundary and return 0 on success, non-zero on failure.
 */
#ifndef FN_APM_C_API_H
#define FN_APM_C_API_H

#if defined(_WIN32)
#define FN_APM_EXPORT __declspec(dllexport)
#else
#define FN_APM_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* fn_apm_handle;

/* Create an APM instance. Returns NULL on failure. */
FN_APM_EXPORT fn_apm_handle fn_apm_create(void);

/* Destroy an instance created by fn_apm_create. Safe to call with NULL. */
FN_APM_EXPORT void fn_apm_destroy(fn_apm_handle handle);

/*
 * Configure processing. Call once after create (may be called again to reconfigure).
 *   sample_rate_hz : stream rate (must be a multiple of 100; 8000..48000 typical)
 *   num_channels   : 1 (mono) for the FnVoice capture path
 *   enable_aec     : echo cancellation on/off (0/1)
 *   enable_ns      : noise suppression on/off (0/1)
 *   enable_agc     : automatic gain control on/off (0/1)
 *   ns_level       : 0 Low, 1 Moderate, 2 High, 3 VeryHigh
 *   agc_mode       : 0 AdaptiveDigital, 1 FixedDigital (AGC1 mode)
 * Returns 0 on success.
 */
FN_APM_EXPORT int fn_apm_configure(fn_apm_handle handle, int sample_rate_hz, int num_channels,
                                   int enable_aec, int enable_ns, int enable_agc, int ns_level, int agc_mode);

/* Process one 10 ms mono capture frame in place. num_samples == sample_rate_hz / 100. Returns 0 on success. */
FN_APM_EXPORT int fn_apm_process_stream(fn_apm_handle handle, float* frame, int num_samples);

/* Process one 10 ms mono render/reference frame (for AEC). num_samples == sample_rate_hz / 100. */
FN_APM_EXPORT int fn_apm_process_reverse_stream(fn_apm_handle handle, float* frame, int num_samples);

/* Set the mic-vs-render delay hint (ms) used by AEC. Returns 0 on success. */
FN_APM_EXPORT int fn_apm_set_stream_delay_ms(fn_apm_handle handle, int delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* FN_APM_C_API_H */
