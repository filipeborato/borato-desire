#pragma once

#include <array>
#include <atomic>
#include <cmath>

#include <juce_dsp/juce_dsp.h>

namespace desire {

// Real-time safe saturation/character engine. All parameter changes are
// smoothed; process() never allocates, locks, or touches the filesystem.
class DesireEngine {
public:
    struct Parameters {
        float inputDb = 0.0f;
        float body = 0.5f;     // 0..1
        float heat = 0.5f;     // 0..1
        float silk = 0.5f;     // 0..1
        float desire = 0.5f;   // 0..1 (main drive)
        float motion = 0.0f;   // 0..1
        float width = 1.0f;    // 0..2 (1 = unity stereo image)
        float mix = 1.0f;      // 0..1
        float outputDb = 0.0f;
        int mode = 1;          // 0 = Intimate, 1 = Club, 2 = After Dark
        bool bypassed = false;
    };

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    // Call once per block before process().
    void setParameters(const Parameters& parameters) noexcept;

    // Processes buffer in place. Channel count must match prepare().
    void process(juce::AudioBuffer<float>& buffer) noexcept;

    // Metering, safe to read from the message thread (relaxed atomics).
    float lastGainReductionDb() const noexcept { return lastGainReductionDb_.load(std::memory_order_relaxed); }
    float lastDesireEnergy() const noexcept { return lastDesireEnergy_.load(std::memory_order_relaxed); }

private:
    // Differentiated ramp times (DesireTuning-equivalent constants): a preset
    // jump moves every parameter at once, and a single short ramp for all of
    // them reads as the sound "resetting" rather than transitioning. Slower
    // parameters (motion/width/desire) get longer ramps so the character
    // moves, rather than jumps, between presets or automation points.
    static constexpr float kInputOutputRampSeconds = 0.035f;  // 20-50ms
    static constexpr float kToneRampSeconds = 0.07f;          // body/heat/silk, 40-100ms
    static constexpr float kDesireRampSeconds = 0.115f;       // 80-150ms
    static constexpr float kMotionRampSeconds = 0.225f;       // 150-300ms
    static constexpr float kWidthRampSeconds = 0.115f;        // 80-150ms
    static constexpr float kMixRampSeconds = 0.035f;          // 20-50ms; also the bypass crossfade
    static constexpr float kModeRampSeconds = 0.2f;           // 100-300ms
    // Was 24dB/6dB: at the "Late Night Confessions" default (desire=76%, body=65%)
    // that put ~12.8x gain into the waveshaper for a signal already near 0dBFS --
    // a mastered/hot track got slammed into constant near-ceiling saturation
    // (squarewave-like, not "warmth") instead of the intended character effect.
    // Halved both so the default preset stays musical on already-loud material;
    // 100% Desire is still a strong, obviously audible drive.
    static constexpr float kMaxDriveDb = 12.0f;
    static constexpr float kSilkMinHz = 5500.0f;
    static constexpr float kSilkMaxHz = 20000.0f;
    static constexpr float kBodyMaxDb = 4.0f;

    // Motion used to modulate drive-gain amplitude at 0.35Hz -- a mono, purely
    // "loudness wobble" effect with no stereo component, which is why turning
    // it produced no perceptible movement. Real motion needs two channels to
    // move relative to each other: two fractional delay lines, phase-offset,
    // slowly modulating delay time (chorus-adjacent but slow enough to read as
    // drift, not pitch wobble). This also gives Width actual decorrelated
    // material to widen instead of near-identical L/R.
    static constexpr float kMotionBaseDelayMs = 3.0f;
    static constexpr float kMotionModDepthMs = 3.0f;
    static constexpr float kMotionLfoHz = 0.18f;

    static float waveshape(float x, float hardness) noexcept {
        const float soft = std::tanh(x);
        const float hard = x / (1.0f + std::abs(x));
        return soft + (hard - soft) * hardness;
    }

    static float modeHardnessBias(int mode) noexcept {
        switch (mode) {
            case 0: return -0.12f;  // Intimate: softer knee
            case 2: return 0.15f;   // After Dark: harder knee, darker
            default: return 0.0f;   // Club
        }
    }

    static float modeBodyBiasDb(int mode) noexcept {
        return mode == 2 ? 2.0f : 0.0f;
    }

    void updateBlockRateCoefficients(int numSamples) noexcept;

    double sampleRate_ = 44100.0;
    int numChannels_ = 2;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGain_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveGain_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> hardness_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> width_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mix_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> motionDepth_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> body_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> silk_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> modeBodyBias_;

    std::array<juce::dsp::IIR::Filter<float>, 2> bodyShelf_;
    std::array<juce::dsp::IIR::Filter<float>, 2> silkLowpass_;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> motionDelayL_;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> motionDelayR_;
    float motionLfoPhase_ = 0.0f;
    float motionLfoIncrement_ = 0.0f;

    std::atomic<float> lastGainReductionDb_ { 0.0f };
    std::atomic<float> lastDesireEnergy_ { 0.0f };
};

} // namespace desire
