#include "DesireEngine.h"

namespace desire {

namespace {
// std::isfinite() rejects NaN/Inf that could otherwise "stick" a SmoothedValue
// ramp forever (e.g. a hand-edited preset XML). Values from getRawParameterValue()
// are host/APVTS-controlled and should already be finite, but this is cheap enough
// to apply unconditionally rather than trust every call site.
float sanitize(float value, float lo, float hi, float fallback) noexcept {
    return std::isfinite(value) ? juce::jlimit(lo, hi, value) : fallback;
}
} // namespace

void DesireEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
    sampleRate_ = sampleRate;
    numChannels_ = juce::jlimit(1, 2, numChannels);

    inputGain_.reset(sampleRate_, kInputOutputRampSeconds);
    outputGain_.reset(sampleRate_, kInputOutputRampSeconds);
    driveGain_.reset(sampleRate_, kDesireRampSeconds);
    hardness_.reset(sampleRate_, kToneRampSeconds);
    width_.reset(sampleRate_, kWidthRampSeconds);
    mix_.reset(sampleRate_, kMixRampSeconds);
    motionDepth_.reset(sampleRate_, kMotionRampSeconds);
    body_.reset(sampleRate_, kToneRampSeconds);
    silk_.reset(sampleRate_, kToneRampSeconds);
    modeBodyBias_.reset(sampleRate_, kModeRampSeconds);

    motionLfoIncrement_ = juce::MathConstants<float>::twoPi * kMotionLfoHz / static_cast<float>(sampleRate_);

    juce::dsp::ProcessSpec spec { sampleRate_, static_cast<juce::uint32>(samplesPerBlock),
                                   static_cast<juce::uint32>(numChannels_) };
    for (auto& filter : bodyShelf_)
        filter.prepare(spec);
    for (auto& filter : silkLowpass_)
        filter.prepare(spec);

    const juce::dsp::ProcessSpec monoSpec { sampleRate_, static_cast<juce::uint32>(samplesPerBlock), 1 };
    const int maxDelaySamples = static_cast<int>(
        (kMotionBaseDelayMs + kMotionModDepthMs) * 0.001 * sampleRate_) + 16;
    motionDelayL_.prepare(monoSpec);
    motionDelayR_.prepare(monoSpec);
    motionDelayL_.setMaximumDelayInSamples(maxDelaySamples);
    motionDelayR_.setMaximumDelayInSamples(maxDelaySamples);

    // Warm the Coefficients' internal Array capacity here (message thread, allocation
    // is fine) so the per-block reassignment in updateBlockRateCoefficients() -- which
    // runs on the audio thread -- never triggers ensureStorageAllocated() to grow.
    const auto warmBody = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
        sampleRate_, 220.0f, 0.707f, 1.0f);
    const auto warmSilk = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
        sampleRate_, kSilkMaxHz);
    for (auto& filter : bodyShelf_)
        *filter.coefficients = warmBody;
    for (auto& filter : silkLowpass_)
        *filter.coefficients = warmSilk;

    reset();
}

void DesireEngine::reset() {
    inputGain_.setCurrentAndTargetValue(1.0f);
    outputGain_.setCurrentAndTargetValue(1.0f);
    driveGain_.setCurrentAndTargetValue(1.0f);
    hardness_.setCurrentAndTargetValue(0.0f);
    width_.setCurrentAndTargetValue(1.0f);
    mix_.setCurrentAndTargetValue(1.0f);
    motionDepth_.setCurrentAndTargetValue(0.0f);
    body_.setCurrentAndTargetValue(0.5f);
    silk_.setCurrentAndTargetValue(0.5f);
    modeBodyBias_.setCurrentAndTargetValue(0.0f);
    motionLfoPhase_ = 0.0f;

    for (auto& filter : bodyShelf_)
        filter.reset();
    for (auto& filter : silkLowpass_)
        filter.reset();
    motionDelayL_.reset();
    motionDelayR_.reset();

    lastGainReductionDb_.store(0.0f, std::memory_order_relaxed);
    lastDesireEnergy_.store(0.0f, std::memory_order_relaxed);
}

void DesireEngine::setParameters(const Parameters& parameters) noexcept {
    const float inputDb = sanitize(parameters.inputDb, -24.0f, 12.0f, 0.0f);
    const float desire = sanitize(parameters.desire, 0.0f, 1.0f, 0.0f);
    const float heat = sanitize(parameters.heat, 0.0f, 1.0f, 0.0f);
    const float width = sanitize(parameters.width, 0.0f, 2.0f, 1.0f);
    const float mix = sanitize(parameters.mix, 0.0f, 1.0f, 0.0f);
    const float motion = sanitize(parameters.motion, 0.0f, 1.0f, 0.0f);
    const float outputDb = sanitize(parameters.outputDb, -24.0f, 12.0f, 0.0f);
    const float body = sanitize(parameters.body, 0.0f, 1.0f, 0.5f);
    const float silk = sanitize(parameters.silk, 0.0f, 1.0f, 0.5f);
    const int mode = (parameters.mode >= 0 && parameters.mode <= 2) ? parameters.mode : 1;

    inputGain_.setTargetValue(juce::Decibels::decibelsToGain(inputDb));

    // True bypass: ramp output trim back to unity too, not just the dry/wet mix,
    // so a non-zero Output knob can't still colour the "bypassed" signal.
    outputGain_.setTargetValue(
        juce::Decibels::decibelsToGain(parameters.bypassed ? 0.0f : outputDb));

    const float driveDb = desire * kMaxDriveDb;
    driveGain_.setTargetValue(juce::Decibels::decibelsToGain(driveDb));

    const float hardness = juce::jlimit(0.0f, 1.0f, heat + modeHardnessBias(mode));
    hardness_.setTargetValue(hardness);

    width_.setTargetValue(width);
    mix_.setTargetValue(parameters.bypassed ? 0.0f : mix);
    motionDepth_.setTargetValue(motion);

    body_.setTargetValue(body);
    silk_.setTargetValue(silk);
    modeBodyBias_.setTargetValue(modeBodyBiasDb(mode));
}

void DesireEngine::updateBlockRateCoefficients(int numSamples) noexcept {
    const float bodyValue = body_.skip(numSamples);
    const float silkValue = silk_.skip(numSamples);
    const float modeBiasDb = modeBodyBias_.skip(numSamples);

    const float bodyDb = bodyValue * kBodyMaxDb + modeBiasDb;
    const float silkCutoff = juce::jmap(silkValue, 0.0f, 1.0f, kSilkMaxHz, kSilkMinHz);

    // ArrayCoefficients returns a stack std::array; Coefficients::operator=(std::array)
    // reuses the Array's existing capacity (warmed in prepare()) instead of allocating
    // a new ref-counted Coefficients object every block -- this runs on the audio thread.
    const auto bodyCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
        sampleRate_, 220.0f, 0.707f, juce::Decibels::decibelsToGain(bodyDb));
    const auto silkCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
        sampleRate_, juce::jlimit(200.0f, static_cast<float>(sampleRate_) * 0.49f, silkCutoff));

    for (auto& filter : bodyShelf_)
        *filter.coefficients = bodyCoeffs;
    for (auto& filter : silkLowpass_)
        *filter.coefficients = silkCoeffs;
}

void DesireEngine::process(juce::AudioBuffer<float>& buffer) noexcept {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(numChannels_, buffer.getNumChannels());
    if (numSamples <= 0 || numChannels <= 0)
        return;

    updateBlockRateCoefficients(numSamples);

    float maxGainReductionDb = 0.0f;
    float energySumSquares = 0.0f;

    std::array<float*, 2> channelData {};
    for (int channel = 0; channel < numChannels; ++channel)
        channelData[static_cast<size_t>(channel)] = buffer.getWritePointer(channel);

    for (int sample = 0; sample < numSamples; ++sample) {
        const float inGain = inputGain_.getNextValue();
        const float drive = driveGain_.getNextValue();
        const float hardness = hardness_.getNextValue();
        const float widthAmount = width_.getNextValue();
        const float mixAmount = mix_.getNextValue();
        const float depth = motionDepth_.getNextValue();
        const float outGain = outputGain_.getNextValue();

        // Motion: two fractional delay lines, phase-offset ~90 degrees between
        // L/R, slowly modulating delay time. depth=0 collapses both to the same
        // fixed delay (transparent -- equal delay on both channels is inaudible
        // as anything but a tiny constant latency); depth>0 makes L and R drift
        // apart in time, which is what actually reads as stereo movement.
        const float motionLfoL = std::sin(motionLfoPhase_);
        const float motionLfoR = std::sin(motionLfoPhase_ + juce::MathConstants<float>::halfPi);
        motionLfoPhase_ += motionLfoIncrement_;
        if (motionLfoPhase_ > juce::MathConstants<float>::twoPi)
            motionLfoPhase_ -= juce::MathConstants<float>::twoPi;

        const float baseDelaySamples = kMotionBaseDelayMs * 0.001f * static_cast<float>(sampleRate_);
        const float modDepthSamples = depth * kMotionModDepthMs * 0.001f * static_cast<float>(sampleRate_);
        const float delayLSamples = baseDelaySamples + modDepthSamples * motionLfoL;
        const float delayRSamples = baseDelaySamples + modDepthSamples * motionLfoR;

        std::array<float, 2> dry {};
        std::array<float, 2> wet {};

        for (int channel = 0; channel < numChannels; ++channel) {
            const float raw = channelData[static_cast<size_t>(channel)][sample];
            dry[static_cast<size_t>(channel)] = raw;

            float x = raw * inGain;
            x = bodyShelf_[static_cast<size_t>(channel)].processSample(x);

            const float driven = x * drive;
            const float shaped = waveshape(driven, hardness);

            const float drivenMagnitude = std::abs(driven);
            if (drivenMagnitude > 1.0e-6f) {
                const float reductionDb = juce::Decibels::gainToDecibels(
                    std::abs(shaped) / drivenMagnitude, -60.0f);
                maxGainReductionDb = juce::jmax(maxGainReductionDb, -reductionDb);
            }
            energySumSquares += shaped * shaped;

            const float afterSilk = silkLowpass_[static_cast<size_t>(channel)].processSample(shaped);

            if (numChannels == 2) {
                auto& delayLine = channel == 0 ? motionDelayL_ : motionDelayR_;
                const float delaySamples = channel == 0 ? delayLSamples : delayRSamples;
                delayLine.pushSample(0, afterSilk);
                wet[static_cast<size_t>(channel)] = delayLine.popSample(0, delaySamples);
            } else {
                wet[static_cast<size_t>(channel)] = afterSilk;
            }
        }

        if (numChannels == 2) {
            const float mid = (wet[0] + wet[1]) * 0.5f;
            const float side = (wet[0] - wet[1]) * 0.5f * widthAmount;
            // Widening decorrelated content above unity (widthAmount > 1) can push
            // |mid +- side| past the waveshaper's own [-1, 1] bound -- tanh() here is
            // a second, cheap soft-knee that keeps the widened signal bounded without
            // a separate limiter, consistent with the rest of the chain's character.
            wet[0] = std::tanh(mid + side);
            wet[1] = std::tanh(mid - side);
        }

        for (int channel = 0; channel < numChannels; ++channel) {
            const float mixed = dry[static_cast<size_t>(channel)]
                * (1.0f - mixAmount) + wet[static_cast<size_t>(channel)] * mixAmount;
            channelData[static_cast<size_t>(channel)][sample] = mixed * outGain;
        }
    }

    lastGainReductionDb_.store(juce::jlimit(0.0f, 24.0f, maxGainReductionDb), std::memory_order_relaxed);
    lastDesireEnergy_.store(
        std::sqrt(energySumSquares / static_cast<float>(numSamples * juce::jmax(1, numChannels))),
        std::memory_order_relaxed);
}

} // namespace desire
