#pragma once

#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "NativeEditorFallback.h"
#include "PluginProcessor.h"
#include "SpectrumAnalyser.h"

namespace desire {

class DesireAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer {
public:
    explicit DesireAudioProcessorEditor(DesireAudioProcessor&);
    ~DesireAudioProcessorEditor() override;

    void resized() override;

private:
    using Resource = juce::WebBrowserComponent::Resource;
    using NativeArgs = juce::Array<juce::var>;
    using NativeCompletion = juce::WebBrowserComponent::NativeFunctionCompletion;

    juce::RangedAudioParameter& parameter(const juce::ParameterID&) const;
    std::optional<Resource> getResource(const juce::String&) const;
    void timerCallback() override;
    void emitVisualFrame();
    void emitStatus();
    void showNativeFallback();
    juce::WebBrowserComponent::Options buildWebViewOptions();
    juce::var buildAnalyzerStateVar() const;
    void applyAnalyzerStateVar(const juce::var&);

    DesireAudioProcessor& processor_;

    juce::WebSliderRelay inputRelay_ { "input" };
    juce::WebSliderRelay bodyRelay_ { "body" };
    juce::WebSliderRelay heatRelay_ { "heat" };
    juce::WebSliderRelay silkRelay_ { "silk" };
    juce::WebSliderRelay desireRelay_ { "desire" };
    juce::WebSliderRelay motionRelay_ { "motion" };
    juce::WebSliderRelay widthRelay_ { "width" };
    juce::WebSliderRelay mixRelay_ { "mix" };
    juce::WebSliderRelay outputRelay_ { "output" };
    juce::WebComboBoxRelay modeRelay_ { "mode" };
    juce::WebToggleButtonRelay bypassRelay_ { "bypass" };

    juce::WebBrowserComponent webView_;
    std::unique_ptr<NativeEditorFallback> fallback_;
    bool webViewReady_ = false;
    bool usingFallback_ = false;
    juce::uint32 startTimeMs_ = 0;

    juce::WebSliderParameterAttachment inputAttachment_;
    juce::WebSliderParameterAttachment bodyAttachment_;
    juce::WebSliderParameterAttachment heatAttachment_;
    juce::WebSliderParameterAttachment silkAttachment_;
    juce::WebSliderParameterAttachment desireAttachment_;
    juce::WebSliderParameterAttachment motionAttachment_;
    juce::WebSliderParameterAttachment widthAttachment_;
    juce::WebSliderParameterAttachment mixAttachment_;
    juce::WebSliderParameterAttachment outputAttachment_;
    juce::WebComboBoxParameterAttachment modeAttachment_;
    juce::WebToggleButtonParameterAttachment bypassAttachment_;

    std::array<float, SpectrumAnalyser::numOutputBins> spectrumBins_ {};
    std::array<float, 3> bandEnergy_ {};
    juce::int64 frameSequence_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesireAudioProcessorEditor)
};

} // namespace desire
