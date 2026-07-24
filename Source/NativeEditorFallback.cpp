#include "NativeEditorFallback.h"

#include "ParameterIDs.h"

namespace desire {

namespace {
void setUpSlider(juce::Slider& slider, juce::Label& label, const juce::String& text,
                  juce::Component& parent) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    parent.addAndMakeVisible(slider);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
    parent.addAndMakeVisible(label);
}
}

NativeEditorFallback::NativeEditorFallback(juce::AudioProcessorValueTreeState& state) {
    title_.setText("Borato Desire", juce::dontSendNotification);
    title_.setJustificationType(juce::Justification::centred);
    title_.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
    addAndMakeVisible(title_);

    diagnostic_.setText(
        "WebView2 UI unavailable -- running native diagnostic controls.",
        juce::dontSendNotification);
    diagnostic_.setJustificationType(juce::Justification::centred);
    diagnostic_.setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(diagnostic_);

    setUpSlider(inputSlider_, inputLabel_, "Input", *this);
    setUpSlider(desireSlider_, desireLabel_, "Desire", *this);
    setUpSlider(mixSlider_, mixLabel_, "Mix", *this);
    setUpSlider(outputSlider_, outputLabel_, "Output", *this);

    addAndMakeVisible(bypassButton_);
    addAndMakeVisible(modeBox_);
    modeBox_.addItemList({ "Intimate", "Club", "After Dark" }, 1);

    inputAttachment_ = std::make_unique<SliderAttachment>(state, param::input.getParamID(), inputSlider_);
    desireAttachment_ = std::make_unique<SliderAttachment>(state, param::desire.getParamID(), desireSlider_);
    mixAttachment_ = std::make_unique<SliderAttachment>(state, param::mix.getParamID(), mixSlider_);
    outputAttachment_ = std::make_unique<SliderAttachment>(state, param::output.getParamID(), outputSlider_);
    bypassAttachment_ = std::make_unique<ButtonAttachment>(state, param::bypass.getParamID(), bypassButton_);
    modeAttachment_ = std::make_unique<ComboBoxAttachment>(state, param::mode.getParamID(), modeBox_);

    setSize(640, 320);
}

void NativeEditorFallback::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(12, 10, 16));
}

void NativeEditorFallback::resized() {
    auto bounds = getLocalBounds().reduced(20);
    title_.setBounds(bounds.removeFromTop(36));
    diagnostic_.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(16);

    auto knobRow = bounds.removeFromTop(140);
    const int knobWidth = knobRow.getWidth() / 4;
    inputSlider_.setBounds(knobRow.removeFromLeft(knobWidth).reduced(12));
    desireSlider_.setBounds(knobRow.removeFromLeft(knobWidth).reduced(12));
    mixSlider_.setBounds(knobRow.removeFromLeft(knobWidth).reduced(12));
    outputSlider_.setBounds(knobRow.removeFromLeft(knobWidth).reduced(12));

    bounds.removeFromTop(20);
    auto controlsRow = bounds.removeFromTop(30);
    bypassButton_.setBounds(controlsRow.removeFromLeft(120));
    modeBox_.setBounds(controlsRow.removeFromLeft(160));
}

} // namespace desire
