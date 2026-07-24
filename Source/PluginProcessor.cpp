#include "PluginProcessor.h"

#include "ParameterIDs.h"
#include "PluginEditor.h"

namespace desire {

namespace {
std::unique_ptr<juce::AudioParameterFloat> percentParameter(
    const juce::ParameterID& id, const juce::String& name, float defaultValue) {
    return std::make_unique<juce::AudioParameterFloat>(
        id, name, juce::NormalisableRange<float>{ 0.0f, 100.0f, 0.1f }, defaultValue,
        juce::AudioParameterFloatAttributes{}.withLabel("%"));
}
}

DesireAudioProcessor::DesireAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state_(*this, &undoManager_, "DesireState", createParameterLayout()),
      presetManager_(state_) {
    slotA_ = state_.copyState();
    slotB_ = state_.copyState();
}

void DesireAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    engine_.prepare(sampleRate, maximumExpectedSamplesPerBlock, getTotalNumOutputChannels());
    spectrumAnalyser_.prepare(sampleRate);
    for (auto& level : meterLevels_)
        level.store(0.0f, std::memory_order_relaxed);
    for (auto& level : rmsLevels_)
        level.store(0.0f, std::memory_order_relaxed);
}

void DesireAudioProcessor::releaseResources() {}

void DesireAudioProcessor::reset() {
    // Hosts call this on transport stop/loop/relocate; clear filter/smoothing
    // state so a paused-and-restarted stream doesn't carry over an IIR tail.
    engine_.reset();
}

void DesireAudioProcessor::pushMonoToAnalyser(const juce::AudioBuffer<float>& buffer) noexcept {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    for (int sample = 0; sample < numSamples; ++sample) {
        float mono = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
            mono += buffer.getSample(channel, sample);
        spectrumAnalyser_.pushSample(numChannels > 0 ? mono / static_cast<float>(numChannels) : 0.0f);
    }
}

bool DesireAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void DesireAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    DesireEngine::Parameters parameters;
    parameters.inputDb = state_.getRawParameterValue(param::input.getParamID())->load();

    // Input meter reflects what's actually being fed into the drive stage, not
    // the raw incoming signal -- that's the whole point of pairing a trim knob
    // with a meter (gauge how hard you're driving it). Reading straight off the
    // buffer here, before engine_.process() applies inputDb at all, meant the
    // Input knob never visibly moved this meter.
    const float inputGainLinear = juce::Decibels::decibelsToGain(parameters.inputDb);
    for (int channel = 0; channel < juce::jmin(2, buffer.getNumChannels()); ++channel) {
        const auto peak = buffer.getMagnitude(channel, 0, buffer.getNumSamples()) * inputGainLinear;
        const auto rms = buffer.getRMSLevel(channel, 0, buffer.getNumSamples()) * inputGainLinear;
        updatePeak(meterLevels_[static_cast<size_t>(channel)], peak);
        rmsLevels_[static_cast<size_t>(channel)].store(rms, std::memory_order_relaxed);
    }

    parameters.body = state_.getRawParameterValue(param::body.getParamID())->load() * 0.01f;
    parameters.heat = state_.getRawParameterValue(param::heat.getParamID())->load() * 0.01f;
    parameters.silk = state_.getRawParameterValue(param::silk.getParamID())->load() * 0.01f;
    parameters.desire = state_.getRawParameterValue(param::desire.getParamID())->load() * 0.01f;
    parameters.motion = state_.getRawParameterValue(param::motion.getParamID())->load() * 0.01f;
    parameters.width = state_.getRawParameterValue(param::width.getParamID())->load() * 0.01f;
    parameters.mix = state_.getRawParameterValue(param::mix.getParamID())->load() * 0.01f;
    parameters.outputDb = state_.getRawParameterValue(param::output.getParamID())->load();
    parameters.mode = static_cast<int>(state_.getRawParameterValue(param::mode.getParamID())->load());
    parameters.bypassed = state_.getRawParameterValue(param::bypass.getParamID())->load() >= 0.5f;

    if (analyzerEnabled_.load(std::memory_order_relaxed) && !analyzerPostTap_.load(std::memory_order_relaxed))
        pushMonoToAnalyser(buffer);

    engine_.setParameters(parameters);
    engine_.process(buffer);

    if (analyzerEnabled_.load(std::memory_order_relaxed) && analyzerPostTap_.load(std::memory_order_relaxed))
        pushMonoToAnalyser(buffer);

    for (int channel = 0; channel < juce::jmin(2, buffer.getNumChannels()); ++channel) {
        const auto peak = buffer.getMagnitude(channel, 0, buffer.getNumSamples());
        const auto rms = buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
        updatePeak(meterLevels_[static_cast<size_t>(channel + 2)], peak);
        rmsLevels_[static_cast<size_t>(channel + 2)].store(rms, std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* DesireAudioProcessor::createEditor() {
    return new DesireAudioProcessorEditor(*this);
}

void DesireAudioProcessor::getStateInformation(juce::MemoryBlock& destination) {
    if (auto xml = state_.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void DesireAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(state_.state.getType()))
            state_.replaceState(juce::ValueTree::fromXml(*xml));
}

std::array<float, 4> DesireAudioProcessor::meterLevels() const noexcept {
    std::array<float, 4> result {};
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = meterLevels_[i].exchange(0.0f, std::memory_order_relaxed);
    return result;
}

void DesireAudioProcessor::selectABSlot(char slot) {
    if (slot != 'A' && slot != 'B')
        return;
    auto& activeSlotTree = activeSlot_ == 'A' ? slotA_ : slotB_;
    activeSlotTree = state_.copyState();

    activeSlot_ = slot;
    auto& targetTree = slot == 'A' ? slotA_ : slotB_;
    if (targetTree.isValid())
        state_.replaceState(targetTree);
}

void DesireAudioProcessor::copyAToB() {
    if (activeSlot_ == 'A')
        slotA_ = state_.copyState();
    slotB_ = slotA_.createCopy();
    if (activeSlot_ == 'B')
        state_.replaceState(slotB_);
}

void DesireAudioProcessor::copyBToA() {
    if (activeSlot_ == 'B')
        slotB_ = state_.copyState();
    slotA_ = slotB_.createCopy();
    if (activeSlot_ == 'A')
        state_.replaceState(slotA_);
}

std::array<float, 4> DesireAudioProcessor::rmsLevels() const noexcept {
    std::array<float, 4> result {};
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = rmsLevels_[i].load(std::memory_order_relaxed);
    return result;
}

void DesireAudioProcessor::updatePeak(std::atomic<float>& destination, float peak) noexcept {
    auto current = destination.load(std::memory_order_relaxed);
    while (current < peak
           && !destination.compare_exchange_weak(current, peak, std::memory_order_relaxed)) {}
}

juce::AudioProcessorValueTreeState::ParameterLayout DesireAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        // -10dB default, not 0dB: with 0dB the default drive stage was slamming
        // already-hot/mastered material straight into ceiling saturation.
        param::input, "Input", juce::NormalisableRange<float>{ -24.0f, 12.0f, 0.1f }, -10.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")));
    layout.add(percentParameter(param::body, "Body", 65.0f));
    layout.add(percentParameter(param::heat, "Heat", 72.0f));
    layout.add(percentParameter(param::silk, "Silk", 58.0f));
    layout.add(percentParameter(param::desire, "Desire", 76.0f));
    layout.add(percentParameter(param::motion, "Motion", 64.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        param::width, "Width", juce::NormalisableRange<float>{ 0.0f, 200.0f, 0.1f }, 128.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("%")));
    layout.add(percentParameter(param::mix, "Mix", 42.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        param::output, "Output", juce::NormalisableRange<float>{ -24.0f, 12.0f, 0.1f }, 0.0f,
        juce::AudioParameterFloatAttributes{}.withLabel("dB")));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        param::mode, "Mode", juce::StringArray{ "Intimate", "Club", "After Dark" }, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(param::bypass, "Bypass", false));
    return layout;
}

} // namespace desire

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new desire::DesireAudioProcessor();
}
