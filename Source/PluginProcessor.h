#pragma once

#include <array>
#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "DesireEngine.h"
#include "PresetManager.h"
#include "SpectrumAnalyser.h"

namespace desire {

class DesireAudioProcessor final : public juce::AudioProcessor {
public:
    DesireAudioProcessor();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState& state() noexcept { return state_; }
    juce::UndoManager& undoManager() noexcept { return undoManager_; }
    std::array<float, 4> meterLevels() const noexcept;
    std::array<float, 4> rmsLevels() const noexcept;
    float gainReductionDb() const noexcept { return engine_.lastGainReductionDb(); }
    float desireEnergy() const noexcept { return engine_.lastDesireEnergy(); }
    SpectrumAnalyser& spectrumAnalyser() noexcept { return spectrumAnalyser_; }

    void setAnalyzerEnabled(bool enabled) noexcept { analyzerEnabled_.store(enabled, std::memory_order_relaxed); }
    void setAnalyzerPostTap(bool post) noexcept { analyzerPostTap_.store(post, std::memory_order_relaxed); }

    PresetManager& presetManager() noexcept { return presetManager_; }

    void selectABSlot(char slot);
    void copyAToB();
    void copyBToA();
    char currentABSlot() const noexcept { return activeSlot_; }

    void undo() { if (undoManager_.canUndo()) undoManager_.undo(); }
    void redo() { if (undoManager_.canRedo()) undoManager_.redo(); }

    // Non-automable UI preferences (analyzer view, reduced motion, ...). Persisted as a
    // child of the APVTS tree so it rides along with getStateInformation/setStateInformation.
    juce::ValueTree uiState() { return state_.state.getOrCreateChildWithName("UI_STATE", nullptr); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static void updatePeak(std::atomic<float>& destination, float peak) noexcept;
    void pushMonoToAnalyser(const juce::AudioBuffer<float>& buffer) noexcept;

    juce::UndoManager undoManager_;
    juce::AudioProcessorValueTreeState state_;
    DesireEngine engine_;
    SpectrumAnalyser spectrumAnalyser_;
    std::atomic<bool> analyzerEnabled_ { true };
    std::atomic<bool> analyzerPostTap_ { true };
    PresetManager presetManager_;
    juce::ValueTree slotA_, slotB_;
    char activeSlot_ = 'A';
    mutable std::array<std::atomic<float>, 4> meterLevels_ {};
    mutable std::array<std::atomic<float>, 4> rmsLevels_ {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesireAudioProcessor)
};

} // namespace desire
