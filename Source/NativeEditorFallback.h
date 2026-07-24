#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace desire {

// Diagnostic / no-WebView2 fallback UI. Deliberately plain -- it exists so the
// plugin is always controllable and never a blank window, not to match the
// WebView interface visually. Covers Input, Desire, Mix, Output, Bypass, Mode.
class NativeEditorFallback final : public juce::Component {
public:
    explicit NativeEditorFallback(juce::AudioProcessorValueTreeState& state);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    juce::Label title_;
    juce::Label diagnostic_;

    juce::Slider inputSlider_, desireSlider_, mixSlider_, outputSlider_;
    juce::Label inputLabel_, desireLabel_, mixLabel_, outputLabel_;
    juce::ToggleButton bypassButton_ { "Bypass" };
    juce::ComboBox modeBox_;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> inputAttachment_, desireAttachment_, mixAttachment_, outputAttachment_;
    std::unique_ptr<ButtonAttachment> bypassAttachment_;
    std::unique_ptr<ComboBoxAttachment> modeAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeEditorFallback)
};

} // namespace desire
