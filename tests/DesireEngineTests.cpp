// Plain-assert DSP validation for DesireEngine. No test framework: a failed
// check aborts with the file/line of the broken invariant. Registered with
// CTest as `DesireEngineTests`.
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>

#include "DesireEngine.h"

using desire::DesireEngine;

// Global new/delete counter, active only while g_trackAllocations is set. Proves
// DesireEngine::process() is actually allocation-free on the audio thread --
// a claim the numeric tests above cannot verify (wrong numbers and a hidden
// malloc/free pair look identical to them).
namespace {
std::atomic<bool> g_trackAllocations { false };
std::atomic<long> g_allocationCount { 0 };
}

void* operator new(std::size_t size) {
    if (g_trackAllocations.load(std::memory_order_relaxed))
        g_allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* ptr = std::malloc(size))
        return ptr;
    throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

// A plain assert() compiles to nothing under NDEBUG (Release), silently
// turning the whole test into a no-op. This check fires in every build type.
#define DESIRE_CHECK(cond)                                                          \
    do {                                                                            \
        if (!(cond)) {                                                              \
            std::fprintf(stderr, "CHECK FAILED: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
            std::abort();                                                           \
        }                                                                           \
    } while (0)

namespace {

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

// Deterministic pseudo-noise so tests are reproducible across runs/platforms.
struct Lcg {
    uint32_t state = 12345;
    float next() noexcept {
        state = state * 1664525u + 1013904223u;
        return (static_cast<float>(state >> 8) / static_cast<float>(1u << 24)) * 2.0f - 1.0f;
    }
};

juce::AudioBuffer<float> makeNoiseBuffer(int numChannels, int numSamples, Lcg& rng, float amplitude = 1.0f) {
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    for (int channel = 0; channel < numChannels; ++channel)
        for (int sample = 0; sample < numSamples; ++sample)
            buffer.setSample(channel, sample, rng.next() * amplitude);
    return buffer;
}

DesireEngine::Parameters extremeParameters(bool loud) {
    DesireEngine::Parameters p;
    p.inputDb = loud ? 12.0f : -24.0f;
    p.body = 1.0f;
    p.heat = 1.0f;
    p.silk = loud ? 0.0f : 1.0f;
    p.desire = 1.0f;
    p.motion = 1.0f;
    p.width = 2.0f;
    p.mix = 1.0f;
    p.outputDb = loud ? 12.0f : -24.0f;
    p.mode = loud ? 2 : 0;
    p.bypassed = false;
    return p;
}

void runBlocks(DesireEngine& engine, const DesireEngine::Parameters& params,
               juce::AudioBuffer<float>& buffer, int numBlocks) {
    for (int i = 0; i < numBlocks; ++i) {
        engine.setParameters(params);
        engine.process(buffer);
    }
}

// Mirrors PresetManager's factory table entry 0, "Late Night Confessions"
// (Source/PresetManager.cpp), converted into DesireEngine::Parameters units.
// Kept in sync manually -- PresetManagerTests.cpp exercises the real
// PresetManager -> APVTS path; this lets the engine-only suite pin the
// same default without linking juce_audio_processors.
DesireEngine::Parameters lateNightConfessionsDefaults() {
    DesireEngine::Parameters p;
    p.inputDb = -10.0f;
    p.body = 0.65f;
    p.heat = 0.72f;
    p.silk = 0.58f;
    p.desire = 0.76f;
    p.motion = 0.64f;
    p.width = 1.28f;
    p.mix = 0.42f;
    p.outputDb = 0.0f;
    p.mode = 1; // Club
    p.bypassed = false;
    return p;
}

void testSilenceInSilenceOut() {
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    DesireEngine::Parameters params = extremeParameters(true);
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    // A few blocks so parameter smoothing settles fully.
    for (int i = 0; i < 10; ++i) {
        engine.setParameters(params);
        engine.process(buffer);
    }

    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < kBlockSize; ++sample)
            DESIRE_CHECK(std::abs(buffer.getSample(channel, sample)) < 1.0e-6f);

    std::puts("[PASS] silence in -> silence out");
}

void testNoNaNOrInfAcrossParameterSweep() {
    Lcg rng;
    const std::array<bool, 2> loudness { false, true };

    for (bool loud : loudness) {
        DesireEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);
        auto params = extremeParameters(loud);
        auto buffer = makeNoiseBuffer(2, kBlockSize, rng, 1.0f);

        runBlocks(engine, params, buffer, 8);

        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < kBlockSize; ++sample) {
                const float value = buffer.getSample(channel, sample);
                DESIRE_CHECK(std::isfinite(value));
                DESIRE_CHECK(std::abs(value) < 20.0f); // sanity bound, not a loudness target
            }
    }

    std::puts("[PASS] finite, bounded output across extreme parameter sweep");
}

void testBypassIsUnityPassthrough() {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    DesireEngine::Parameters params;
    params.inputDb = 6.0f;   // would audibly drive the shaper if not bypassed
    params.desire = 1.0f;
    params.heat = 1.0f;
    params.outputDb = 0.0f;
    params.mix = 1.0f;
    params.bypassed = true;

    auto reference = makeNoiseBuffer(2, kBlockSize, rng, 0.5f);
    juce::AudioBuffer<float> buffer;
    buffer.makeCopyOf(reference);

    // Feed silence first so the bypass mix-to-zero ramp fully settles before
    // the signal under test arrives.
    juce::AudioBuffer<float> silence(2, kBlockSize);
    silence.clear();
    runBlocks(engine, params, silence, 10);

    engine.setParameters(params);
    engine.process(buffer);

    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < kBlockSize; ++sample)
            DESIRE_CHECK(std::abs(buffer.getSample(channel, sample) - reference.getSample(channel, sample)) < 1.0e-3f);

    std::puts("[PASS] bypass is a unity passthrough once the mix ramp settles");
}

void testBypassIgnoresOutputTrim() {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    DesireEngine::Parameters params;
    params.inputDb = 6.0f;
    params.desire = 1.0f;
    params.heat = 1.0f;
    params.outputDb = 9.0f;   // a non-zero trim must NOT leak through while bypassed
    params.mix = 1.0f;
    params.bypassed = true;

    auto reference = makeNoiseBuffer(2, kBlockSize, rng, 0.5f);
    juce::AudioBuffer<float> buffer;
    buffer.makeCopyOf(reference);

    juce::AudioBuffer<float> silence(2, kBlockSize);
    silence.clear();
    runBlocks(engine, params, silence, 10);

    engine.setParameters(params);
    engine.process(buffer);

    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < kBlockSize; ++sample)
            DESIRE_CHECK(std::abs(buffer.getSample(channel, sample) - reference.getSample(channel, sample)) < 1.0e-3f);

    std::puts("[PASS] bypass ignores output trim, not just drive");
}

void testAntiPhaseWidthStaysBounded() {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    DesireEngine::Parameters params = extremeParameters(true);
    params.width = 2.0f;      // maximum side boost
    params.outputDb = 12.0f;  // maximum output trim on top of it

    // Fully anti-phase, full-scale input: R = -L. This is the worst case for the
    // mid/side widener (side = (L - R) / 2 * width), which is what let output
    // exceed 0 dBFS by a wide margin before the post-width tanh() safety stage.
    auto left = makeNoiseBuffer(1, kBlockSize, rng, 1.0f);
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.copyFrom(0, 0, left, 0, 0, kBlockSize);
    for (int sample = 0; sample < kBlockSize; ++sample)
        buffer.setSample(1, sample, -left.getSample(0, sample));

    // width_ now ramps over ~115ms (DesireTuning-style per-parameter smoothing,
    // not one shared 20ms ramp) -- warm up long enough for it to actually reach
    // widthAmount=2.0, or this test would only ever see a partially-ramped,
    // less-than-worst-case value and never actually exercise the ceiling.
    runBlocks(engine, params, buffer, 30);

    // outputDb=12 is ~3.98x; the waveshaper/width stage is bounded to [-1, 1], so
    // the physical ceiling here is that gain, with slack for the mix blend with dry.
    const float ceiling = juce::Decibels::decibelsToGain(12.0f) * 1.05f;
    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < kBlockSize; ++sample) {
            const float value = buffer.getSample(channel, sample);
            DESIRE_CHECK(std::isfinite(value));
            DESIRE_CHECK(std::abs(value) < ceiling);
        }

    std::puts("[PASS] anti-phase input at width=200% stays bounded, not just finite");
}

void testZeroMixIsDryPassthrough() {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    DesireEngine::Parameters params;
    params.inputDb = 9.0f;
    params.desire = 1.0f;
    params.heat = 1.0f;
    params.outputDb = 0.0f;
    params.mix = 0.0f;
    params.bypassed = false;

    auto reference = makeNoiseBuffer(2, kBlockSize, rng, 0.5f);
    juce::AudioBuffer<float> buffer;
    buffer.makeCopyOf(reference);

    juce::AudioBuffer<float> silence(2, kBlockSize);
    silence.clear();
    runBlocks(engine, params, silence, 10);

    engine.setParameters(params);
    engine.process(buffer);

    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < kBlockSize; ++sample)
            DESIRE_CHECK(std::abs(buffer.getSample(channel, sample) - reference.getSample(channel, sample)) < 1.0e-3f);

    std::puts("[PASS] mix=0 is a dry passthrough regardless of drive");
}

void testCorrelatedInputStaysMonoCompatibleAtAnyWidth() {
    Lcg rng;
    for (float widthValue : { 0.0f, 1.0f, 2.0f }) {
        DesireEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);

        DesireEngine::Parameters params = extremeParameters(true);
        params.width = widthValue;
        // Motion intentionally decorrelates L/R (that's what makes it audible --
        // see testMotionActuallyDecorrelatesStereoChannels). Isolate Width's own
        // invariant from Motion's by turning Motion off here.
        params.motion = 0.0f;

        auto mono = makeNoiseBuffer(1, kBlockSize, rng, 0.7f);
        juce::AudioBuffer<float> buffer(2, kBlockSize);
        buffer.copyFrom(0, 0, mono, 0, 0, kBlockSize);
        buffer.copyFrom(1, 0, mono, 0, 0, kBlockSize);

        runBlocks(engine, params, buffer, 6);

        for (int sample = 0; sample < kBlockSize; ++sample)
            DESIRE_CHECK(std::abs(buffer.getSample(0, sample) - buffer.getSample(1, sample)) < 1.0e-4f);
    }

    std::puts("[PASS] identical L/R input stays identical regardless of width");
}

// Regression test for the real-world report: turning Motion produced no
// audible change. The old implementation modulated drive-gain amplitude at
// 0.35Hz -- identical on both channels, so it was a mono loudness wobble with
// no stereo component at all. The fix drives L and R through independent,
// phase-offset fractional delay lines, so Motion should measurably decorrelate
// otherwise-identical channels (and stay transparent at motion=0).
void testMotionActuallyDecorrelatesStereoChannels() {
    Lcg rng;
    auto mono = makeNoiseBuffer(1, kBlockSize, rng, 0.7f);

    auto renderWithMotion = [&](float motionAmount) {
        DesireEngine engine;
        engine.prepare(kSampleRate, kBlockSize, 2);

        DesireEngine::Parameters params;
        params.desire = 0.3f; // keep the waveshaper mild so it doesn't mask the delay effect
        params.motion = motionAmount;
        params.mix = 1.0f;

        juce::AudioBuffer<float> buffer(2, kBlockSize);
        buffer.copyFrom(0, 0, mono, 0, 0, kBlockSize);
        buffer.copyFrom(1, 0, mono, 0, 0, kBlockSize);
        runBlocks(engine, params, buffer, 30); // let the slow motion LFO move
        return buffer;
    };

    auto withMotionOff = renderWithMotion(0.0f);
    for (int sample = 0; sample < kBlockSize; ++sample)
        DESIRE_CHECK(std::abs(withMotionOff.getSample(0, sample) - withMotionOff.getSample(1, sample)) < 1.0e-4f);

    auto withMotionOn = renderWithMotion(1.0f);
    double maxChannelDifference = 0.0;
    for (int sample = 0; sample < kBlockSize; ++sample)
        maxChannelDifference = juce::jmax(maxChannelDifference,
            static_cast<double>(std::abs(withMotionOn.getSample(0, sample) - withMotionOn.getSample(1, sample))));

    std::fprintf(stderr, "motion=1.0 max L/R difference=%f\n", maxChannelDifference);
    DESIRE_CHECK(maxChannelDifference > 1.0e-3f);

    std::puts("[PASS] motion decorrelates L/R (transparent at 0, measurable at 100%)");
}

void testProcessNeverAllocatesOnTheAudioThread() {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    DesireEngine::Parameters params = extremeParameters(true);
    auto buffer = makeNoiseBuffer(2, kBlockSize, rng, 1.0f);

    // Warm-up: settle smoothing ramps and let any lazy first-touch allocation
    // (e.g. inside JUCE internals on first use) happen before we start counting.
    runBlocks(engine, params, buffer, 10);

    g_allocationCount.store(0, std::memory_order_relaxed);
    g_trackAllocations.store(true, std::memory_order_relaxed);

    for (int block = 0; block < 20; ++block) {
        params.desire = (block % 2 == 0) ? 0.0f : 1.0f;   // exercise both filter-coefficient paths
        params.body = (block % 3 == 0) ? 0.0f : 1.0f;
        params.mode = block % 3;
        engine.setParameters(params);
        engine.process(buffer);
    }

    g_trackAllocations.store(false, std::memory_order_relaxed);
    const long allocations = g_allocationCount.load(std::memory_order_relaxed);
    DESIRE_CHECK(allocations == 0);

    std::puts("[PASS] process() performs zero heap allocations across 20 varied blocks");
}

// Regression test for the real-world report: an already-mastered (hot, dense)
// track fed into the default "Late Night Confessions" preset came out
// noticeably louder and squashed-sounding -- the drive stage was slamming
// already-loud material into near-constant ceiling saturation. This doesn't
// judge "sounds good" (can't, no ears here), but it does pin two objective,
// checkable proxies for "blew up": RMS shouldn't jump by more than a few dB,
// and the crest factor (peak/RMS) shouldn't collapse into square-wave territory.
void testDefaultPresetDoesNotBlowUpAlreadyHotMaterial() {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(kSampleRate, kBlockSize, 2);

    const auto params = lateNightConfessionsDefaults();

    // A hot, dense "mastered track" stand-in: full-band noise sitting close to
    // 0 dBFS (0.85 peak), much denser/louder on average than a bare sine.
    constexpr float kHotAmplitude = 0.85f;
    constexpr int kBlocks = 40;
    constexpr int kSettleBlocks = kBlocks / 2;

    double inputSumSquares = 0.0, outputSumSquares = 0.0;
    float inputPeak = 0.0f, outputPeak = 0.0f;
    long total = 0;

    for (int b = 0; b < kBlocks; ++b) {
        auto buffer = makeNoiseBuffer(2, kBlockSize, rng, kHotAmplitude);
        juce::AudioBuffer<float> reference;
        reference.makeCopyOf(buffer);

        engine.setParameters(params);
        engine.process(buffer);

        if (b >= kSettleBlocks) { // only measure once smoothing ramps have settled
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < kBlockSize; ++i) {
                    const float in = reference.getSample(ch, i);
                    const float out = buffer.getSample(ch, i);
                    inputSumSquares += static_cast<double>(in) * in;
                    outputSumSquares += static_cast<double>(out) * out;
                    inputPeak = juce::jmax(inputPeak, std::abs(in));
                    outputPeak = juce::jmax(outputPeak, std::abs(out));
                    ++total;
                }
            }
        }
    }

    const float inputRms = static_cast<float>(std::sqrt(inputSumSquares / static_cast<double>(total)));
    const float outputRms = static_cast<float>(std::sqrt(outputSumSquares / static_cast<double>(total)));
    const float rmsGrowthDb = juce::Decibels::gainToDecibels(outputRms / inputRms, -60.0f);
    const float outputCrestFactor = outputPeak / outputRms;

    std::fprintf(stderr, "inputRms=%f outputRms=%f rmsGrowthDb=%f outputCrestFactor=%f\n",
                 inputRms, outputRms, rmsGrowthDb, outputCrestFactor);

    // A "character" effect can add some perceived loudness, but +3dB of RMS
    // growth on already-hot material, at the plugin's own default, is the
    // "distorts on a mastered track" complaint turning into a number.
    DESIRE_CHECK(rmsGrowthDb < 3.0f);
    // A hard-clipped/squarewave-like signal has a crest factor approaching 1.0;
    // this default should still sound like saturation, not a brick-wall clip.
    DESIRE_CHECK(outputCrestFactor > 1.3f);

    std::puts("[PASS] default preset doesn't blow up already-hot material");
}

} // namespace

int main() {
    testSilenceInSilenceOut();
    testNoNaNOrInfAcrossParameterSweep();
    testBypassIsUnityPassthrough();
    testBypassIgnoresOutputTrim();
    testAntiPhaseWidthStaysBounded();
    testZeroMixIsDryPassthrough();
    testCorrelatedInputStaysMonoCompatibleAtAnyWidth();
    testMotionActuallyDecorrelatesStereoChannels();
    testProcessNeverAllocatesOnTheAudioThread();
    testDefaultPresetDoesNotBlowUpAlreadyHotMaterial();
    std::puts("All DesireEngine tests passed.");
    return 0;
}
