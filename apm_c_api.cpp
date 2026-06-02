/*
 * apm_c_api.cpp — implementation of the flat C ABI over webrtc-audio-processing.
 *
 * Target: webrtc-audio-processing v1.x / v2.x (the PulseAudio fork; pkg-config module
 * "webrtc-audio-processing-1" or "webrtc-audio-processing-2"). The float ProcessStream / ProcessReverseStream
 * signatures and the AudioProcessing::Config struct used below are stable across those versions. If you build
 * against the full upstream WebRTC tree the include path differs (webrtc/modules/...), but the API matches.
 *
 * Build: see CMakeLists.txt + README.md in this folder. Produces the "WebRtcApm" shared library that
 * Unity loads (WebRtcApm.dll / libWebRtcApm.so / WebRtcApm.bundle / libWebRtcApm.a).
 */
#include "apm_c_api.h"

#include <modules/audio_processing/include/audio_processing.h>

#include <new>

using webrtc::AudioProcessing;
using webrtc::AudioProcessingBuilder;
using webrtc::StreamConfig;

namespace {

struct ApmInstance {
    rtc::scoped_refptr<AudioProcessing> apm;
    int sample_rate = 16000;
    int channels = 1;
};

inline ApmInstance* as_instance(fn_apm_handle h) { return static_cast<ApmInstance*>(h); }

} // namespace

extern "C" {

FN_APM_EXPORT fn_apm_handle fn_apm_create(void) {
    ApmInstance* inst = new (std::nothrow) ApmInstance();
    if (inst == nullptr) return nullptr;
    inst->apm = AudioProcessingBuilder().Create();
    if (inst->apm == nullptr) {
        delete inst;
        return nullptr;
    }
    return inst;
}

FN_APM_EXPORT void fn_apm_destroy(fn_apm_handle handle) {
    if (handle == nullptr) return;
    delete as_instance(handle);
}

FN_APM_EXPORT int fn_apm_configure(fn_apm_handle handle, int sample_rate_hz, int num_channels,
                                   int enable_aec, int enable_ns, int enable_agc, int ns_level, int agc_mode) {
    if (handle == nullptr) return -1;
    ApmInstance* inst = as_instance(handle);
    inst->sample_rate = sample_rate_hz;
    inst->channels = num_channels;

    AudioProcessing::Config config;

    config.echo_canceller.enabled = enable_aec != 0;
    config.echo_canceller.mobile_mode = false;

    config.noise_suppression.enabled = enable_ns != 0;
    switch (ns_level) {
        case 0:  config.noise_suppression.level = AudioProcessing::Config::NoiseSuppression::kLow; break;
        case 1:  config.noise_suppression.level = AudioProcessing::Config::NoiseSuppression::kModerate; break;
        case 3:  config.noise_suppression.level = AudioProcessing::Config::NoiseSuppression::kVeryHigh; break;
        case 2:
        default: config.noise_suppression.level = AudioProcessing::Config::NoiseSuppression::kHigh; break;
    }

    // AGC1 drives the float capture path (AdaptiveDigital, since there is no analog mic gain to control here).
    config.gain_controller1.enabled = enable_agc != 0;
    config.gain_controller1.mode = (agc_mode == 1)
        ? AudioProcessing::Config::GainController1::kFixedDigital
        : AudioProcessing::Config::GainController1::kAdaptiveDigital;

    config.high_pass_filter.enabled = true;

    inst->apm->ApplyConfig(config);
    return 0;
}

FN_APM_EXPORT int fn_apm_process_stream(fn_apm_handle handle, float* frame, int num_samples) {
    if (handle == nullptr || frame == nullptr) return -1;
    ApmInstance* inst = as_instance(handle);
    StreamConfig cfg(inst->sample_rate, inst->channels);
    float* channels[1] = { frame };
    return inst->apm->ProcessStream(channels, cfg, cfg, channels);
}

FN_APM_EXPORT int fn_apm_process_reverse_stream(fn_apm_handle handle, float* frame, int num_samples) {
    if (handle == nullptr || frame == nullptr) return -1;
    ApmInstance* inst = as_instance(handle);
    StreamConfig cfg(inst->sample_rate, inst->channels);
    float* channels[1] = { frame };
    return inst->apm->ProcessReverseStream(channels, cfg, cfg, channels);
}

FN_APM_EXPORT int fn_apm_set_stream_delay_ms(fn_apm_handle handle, int delay_ms) {
    if (handle == nullptr) return -1;
    as_instance(handle)->apm->set_stream_delay_ms(delay_ms);
    return 0;
}

} // extern "C"
