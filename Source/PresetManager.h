#pragma once

#include <array>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace desire {

// Factory presets are a fixed, hard-coded table (no JSON asset pipeline exists
// for Source/ yet, and nine floats + a mode index per preset isn't worth one).
// User presets are individual XML files (the same shape APVTS already emits)
// dropped in the user's app-data folder, so File Explorer / a DAW's preset
// browser both work for free.
class PresetManager {
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& state);

    juce::StringArray factoryPresetNames() const;
    juce::StringArray userPresetNames() const;

    void selectFactory(int index);
    bool selectUser(const juce::String& name);
    void saveUser(const juce::String& name);
    void deleteUser(const juce::String& name);

    void selectNext();
    void selectPrevious();

    juce::String currentPresetName() const noexcept { return currentName_; }
    juce::File userPresetsFolder() const noexcept { return presetsFolder_; }

private:
    struct FactoryPreset {
        const char* name;
        std::array<float, 9> values; // input,body,heat,silk,desire,motion,width,mix,output
        int mode;
    };

    static const std::vector<FactoryPreset>& factoryTable();

    juce::StringArray combinedNames() const;
    void applyFactory(const FactoryPreset& preset);

    juce::AudioProcessorValueTreeState& state_;
    juce::File presetsFolder_;
    juce::String currentName_ { "Late Night Confessions" };
};

} // namespace desire
