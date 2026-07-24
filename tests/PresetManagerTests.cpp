// Empirically verifies the reported "presets aren't changing the sound" bug
// by driving PresetManager through the exact production code path (APVTS ->
// getRawParameterValue -> DesireEngine::Parameters -> DesireEngine::process)
// and measuring whether the audio, and the underlying parameter values,
// actually differ between presets. No test framework: plain asserts.
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DesireEngine.h"
#include "ParameterIDs.h"
#include "PresetManager.h"

using namespace desire;

#define DESIRE_CHECK(cond)                                                                  \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            std::fprintf(stderr, "CHECK FAILED: %s (%s:%d)\n", #cond, __FILE__, __LINE__);  \
            std::abort();                                                                   \
        }                                                                                    \
    } while (0)

namespace {

// Minimal stand-in for DesireAudioProcessor that doesn't drag in the WebView
// editor -- this test only needs the APVTS + PresetManager + DesireEngine path.
class TestProcessor final : public juce::AudioProcessor {
public:
    TestProcessor()
        : AudioProcessor(BusesProperties()
                              .withInput("Input", juce::AudioChannelSet::stereo(), true)
                              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          state(*this, nullptr, "TestState", createLayout()) {}

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "Test"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout() {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            param::input, "Input", juce::NormalisableRange<float> { -24.0f, 12.0f, 0.1f }, 0.0f));
        for (const auto& id : { &param::body, &param::heat, &param::silk, &param::desire, &param::motion, &param::mix })
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                *id, id->getParamID(), juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 50.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            param::width, "Width", juce::NormalisableRange<float> { 0.0f, 200.0f, 0.1f }, 100.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            param::output, "Output", juce::NormalisableRange<float> { -24.0f, 12.0f, 0.1f }, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            param::mode, "Mode", juce::StringArray { "Intimate", "Club", "After Dark" }, 1));
        layout.add(std::make_unique<juce::AudioParameterBool>(param::bypass, "Bypass", false));
        return layout;
    }

    juce::AudioProcessorValueTreeState state;
};

struct Lcg {
    uint32_t s = 777;
    float next() noexcept {
        s = s * 1664525u + 1013904223u;
        return (static_cast<float>(s >> 8) / static_cast<float>(1u << 24)) * 2.0f - 1.0f;
    }
};

DesireEngine::Parameters readSnapshot(juce::AudioProcessorValueTreeState& state) {
    DesireEngine::Parameters p;
    p.inputDb = state.getRawParameterValue(param::input.getParamID())->load();
    p.body = state.getRawParameterValue(param::body.getParamID())->load() * 0.01f;
    p.heat = state.getRawParameterValue(param::heat.getParamID())->load() * 0.01f;
    p.silk = state.getRawParameterValue(param::silk.getParamID())->load() * 0.01f;
    p.desire = state.getRawParameterValue(param::desire.getParamID())->load() * 0.01f;
    p.motion = state.getRawParameterValue(param::motion.getParamID())->load() * 0.01f;
    p.width = state.getRawParameterValue(param::width.getParamID())->load() * 0.01f;
    p.mix = state.getRawParameterValue(param::mix.getParamID())->load() * 0.01f;
    p.outputDb = state.getRawParameterValue(param::output.getParamID())->load();
    p.mode = static_cast<int>(state.getRawParameterValue(param::mode.getParamID())->load());
    p.bypassed = state.getRawParameterValue(param::bypass.getParamID())->load() >= 0.5f;
    return p;
}

float renderRms(const DesireEngine::Parameters& params, int blocks, int blockSize, double sampleRate) {
    Lcg rng;
    DesireEngine engine;
    engine.prepare(sampleRate, blockSize, 2);

    double sumSquares = 0.0;
    long total = 0;
    for (int b = 0; b < blocks; ++b) {
        juce::AudioBuffer<float> buffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < blockSize; ++i)
                buffer.setSample(ch, i, rng.next() * 0.5f);

        engine.setParameters(params);
        engine.process(buffer);

        if (b >= blocks / 2) { // only measure after the smoothing ramps have settled
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i) {
                    const float v = buffer.getSample(ch, i);
                    sumSquares += static_cast<double>(v) * v;
                    ++total;
                }
        }
    }
    return static_cast<float>(std::sqrt(sumSquares / static_cast<double>(total)));
}

void testFactoryPresetsActuallyChangeParameterValues() {
    TestProcessor processor;
    PresetManager presets(processor.state);

    presets.selectFactory(0);
    const auto snapshotInit = readSnapshot(processor.state);

    presets.selectFactory(2); // "Main Floor"
    const auto snapshotMainFloor = readSnapshot(processor.state);

    bool anyDiffers = false;
    anyDiffers |= std::abs(snapshotInit.body - snapshotMainFloor.body) > 1.0e-4f;
    anyDiffers |= std::abs(snapshotInit.heat - snapshotMainFloor.heat) > 1.0e-4f;
    anyDiffers |= std::abs(snapshotInit.desire - snapshotMainFloor.desire) > 1.0e-4f;
    anyDiffers |= std::abs(snapshotInit.width - snapshotMainFloor.width) > 1.0e-4f;
    anyDiffers |= snapshotInit.mode != snapshotMainFloor.mode;

    DESIRE_CHECK(anyDiffers);
    std::puts("[PASS] selectFactory() actually changes the underlying APVTS parameter values");
}

void testFactoryPresetsActuallyChangeTheAudio() {
    TestProcessor processor;
    PresetManager presets(processor.state);

    presets.selectFactory(0); // "Late Night Confessions"
    const auto snapshotInit = readSnapshot(processor.state);
    const float rmsInit = renderRms(snapshotInit, 40, 512, 48000.0);

    presets.selectFactory(3); // "After Hours" -- deliberately the most different from Init
    const auto snapshotAfterHours = readSnapshot(processor.state);
    const float rmsAfterHours = renderRms(snapshotAfterHours, 40, 512, 48000.0);

    std::fprintf(stderr, "rmsInit=%f rmsAfterHours=%f\n", rmsInit, rmsAfterHours);
    DESIRE_CHECK(std::abs(rmsInit - rmsAfterHours) > 1.0e-4f);
    std::puts("[PASS] different factory presets render audibly different audio");
}

void testSelectNextCyclesThroughAllFactoryPresetsWithoutGettingStuck() {
    TestProcessor processor;
    PresetManager presets(processor.state);

    presets.selectFactory(0);
    DESIRE_CHECK(presets.currentPresetName() == "Late Night Confessions");

    juce::StringArray visited;
    for (int i = 0; i < 4; ++i) {
        visited.add(presets.currentPresetName());
        presets.selectNext();
    }

    // Should visit 4 distinct factory names, not collapse back onto "Late Night Confessions" early.
    visited.removeDuplicates(false);
    DESIRE_CHECK(visited.size() == 4);

    // A full cycle (4x next from Init) must land back on Init.
    DESIRE_CHECK(presets.currentPresetName() == "Late Night Confessions");
    std::puts("[PASS] selectNext() cycles through all factory presets without getting stuck");
}

} // namespace

int main() {
    testFactoryPresetsActuallyChangeParameterValues();
    testFactoryPresetsActuallyChangeTheAudio();
    testSelectNextCyclesThroughAllFactoryPresetsWithoutGettingStuck();
    std::puts("All PresetManager tests passed.");
    return 0;
}
