#include "PresetManager.h"

#include "ParameterIDs.h"

namespace desire {

namespace {
constexpr std::array<const juce::ParameterID*, 9> kOrderedParams {
    &param::input, &param::body, &param::heat, &param::silk, &param::desire,
    &param::motion, &param::width, &param::mix, &param::output
};
}

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state) : state_(state) {
    presetsFolder_ = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("Borato Company")
                          .getChildFile("Borato Desire")
                          .getChildFile("Presets");
    presetsFolder_.createDirectory();
}

const std::vector<PresetManager::FactoryPreset>& PresetManager::factoryTable() {
    // Input trim is -10dB across the board (was 0/2/4/0dB): at 0dB, hot/mastered
    // source material got driven straight into ceiling saturation by default.
    // Relative offsets between presets are preserved, just shifted down 10dB.
    static const std::vector<FactoryPreset> table {
        { "Late Night Confessions", { -10.0f, 65.0f, 72.0f, 58.0f, 76.0f, 64.0f, 128.0f, 42.0f, 0.0f }, 1 },
        { "Velvet Room", { -8.0f, 58.0f, 38.0f, 70.0f, 42.0f, 20.0f, 110.0f, 55.0f, 1.0f }, 0 },
        { "Main Floor", { -6.0f, 70.0f, 80.0f, 45.0f, 82.0f, 55.0f, 150.0f, 65.0f, -1.0f }, 1 },
        { "After Hours", { -10.0f, 75.0f, 90.0f, 35.0f, 95.0f, 70.0f, 130.0f, 78.0f, -2.0f }, 2 },
    };
    return table;
}

juce::StringArray PresetManager::factoryPresetNames() const {
    juce::StringArray names;
    for (const auto& preset : factoryTable())
        names.add(preset.name);
    return names;
}

juce::StringArray PresetManager::userPresetNames() const {
    juce::StringArray names;
    for (const auto& file : presetsFolder_.findChildFiles(juce::File::findFiles, false, "*.xml"))
        names.add(file.getFileNameWithoutExtension());
    names.sort(true);
    return names;
}

juce::StringArray PresetManager::combinedNames() const {
    auto names = factoryPresetNames();
    names.addArray(userPresetNames());
    return names;
}

void PresetManager::applyFactory(const FactoryPreset& preset) {
    for (size_t i = 0; i < kOrderedParams.size(); ++i)
        if (auto* p = state_.getParameter(kOrderedParams[i]->getParamID()))
            p->setValueNotifyingHost(p->convertTo0to1(preset.values[i]));
    if (auto* mode = state_.getParameter(param::mode.getParamID()))
        mode->setValueNotifyingHost(mode->convertTo0to1(static_cast<float>(preset.mode)));
    currentName_ = preset.name;
}

void PresetManager::selectFactory(int index) {
    const auto& table = factoryTable();
    if (index < 0 || index >= static_cast<int>(table.size()))
        return;
    applyFactory(table[static_cast<size_t>(index)]);
}

bool PresetManager::selectUser(const juce::String& name) {
    const auto file = presetsFolder_.getChildFile(name + ".xml");
    if (!file.existsAsFile())
        return false;
    if (auto xml = juce::XmlDocument::parse(file))
        if (xml->hasTagName(state_.state.getType())) {
            state_.replaceState(juce::ValueTree::fromXml(*xml));
            currentName_ = name;
            return true;
        }
    return false;
}

void PresetManager::saveUser(const juce::String& name) {
    if (name.isEmpty())
        return;
    if (auto xml = state_.copyState().createXml())
        xml->writeTo(presetsFolder_.getChildFile(name + ".xml"));
    currentName_ = name;
}

void PresetManager::deleteUser(const juce::String& name) {
    presetsFolder_.getChildFile(name + ".xml").deleteFile();
    if (currentName_ == name)
        currentName_ = "Late Night Confessions";
}

void PresetManager::selectNext() {
    const auto names = combinedNames();
    if (names.isEmpty())
        return;
    const int current = names.indexOf(currentName_);
    const int next = (current + 1) % names.size();
    const auto& factory = factoryTable();
    if (next < static_cast<int>(factory.size()))
        selectFactory(next);
    else
        selectUser(names[next]);
}

void PresetManager::selectPrevious() {
    const auto names = combinedNames();
    if (names.isEmpty())
        return;
    const int current = names.indexOf(currentName_);
    const int previous = (current - 1 + names.size()) % names.size();
    const auto& factory = factoryTable();
    if (previous < static_cast<int>(factory.size()))
        selectFactory(previous);
    else
        selectUser(names[previous]);
}

} // namespace desire
