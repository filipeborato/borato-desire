#include "PluginEditor.h"

#include <unordered_map>
#include <vector>

#include <DesireWebViewFiles.h>

namespace desire {

namespace {

std::vector<std::byte> readStream(juce::InputStream& stream) {
    std::vector<std::byte> result(static_cast<size_t>(stream.getTotalLength()));
    stream.setPosition(0);
    [[maybe_unused]] const auto bytesRead = stream.read(result.data(), result.size());
    jassert(static_cast<size_t>(bytesRead) == result.size());
    return result;
}

const char* mimeTypeFor(const juce::String& extension) {
    static const std::unordered_map<juce::String, const char*> types {
        { "html", "text/html" }, { "css", "text/css" },
        { "js", "text/javascript" }, { "json", "application/json" },
        { "svg", "image/svg+xml" }, { "png", "image/png" },
        { "jpg", "image/jpeg" }, { "jpeg", "image/jpeg" },
        { "webp", "image/webp" }, { "woff2", "font/woff2" },
        { "map", "application/json" }
    };

    if (const auto found = types.find(extension.toLowerCase()); found != types.end())
        return found->second;
    return "application/octet-stream";
}

std::vector<std::byte> resourceBytes(const juce::String& path) {
    juce::MemoryInputStream zipStream(desire_webview_files::desire_webview_zip,
                                      desire_webview_files::desire_webview_zipSize,
                                      false);
    juce::ZipFile zip(zipStream);
    if (const auto* entry = zip.getEntry(DESIRE_WEBVIEW_ZIP_PREFIX + path)) {
        if (auto stream = std::unique_ptr<juce::InputStream>(zip.createStreamForEntry(*entry)))
            return readStream(*stream);
    }
    return {};
}

} // namespace

DesireAudioProcessorEditor::DesireAudioProcessorEditor(DesireAudioProcessor& processor)
    : AudioProcessorEditor(processor),
      processor_(processor),
      webView_([this] { return buildWebViewOptions(); }()),
      inputAttachment_(parameter({ "input", 1 }), inputRelay_),
      bodyAttachment_(parameter({ "body", 1 }), bodyRelay_),
      heatAttachment_(parameter({ "heat", 1 }), heatRelay_),
      silkAttachment_(parameter({ "silk", 1 }), silkRelay_),
      desireAttachment_(parameter({ "desire", 1 }), desireRelay_),
      motionAttachment_(parameter({ "motion", 1 }), motionRelay_),
      widthAttachment_(parameter({ "width", 1 }), widthRelay_),
      mixAttachment_(parameter({ "mix", 1 }), mixRelay_),
      outputAttachment_(parameter({ "output", 1 }), outputRelay_),
      modeAttachment_(parameter({ "mode", 1 }), modeRelay_),
      bypassAttachment_(parameter({ "bypass", 1 }), bypassRelay_) {
    startTimeMs_ = juce::Time::getMillisecondCounter();

#if defined(DESIRE_UI_BACKEND_NATIVE)
    showNativeFallback();
#else
    addAndMakeVisible(webView_);
    webView_.goToURL(juce::WebBrowserComponent::getResourceProviderRoot() + "Prototype/");
#endif

    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(1456.0 / 1024.0);
    setResizeLimits(728, 512, 2912, 2048);
    setSize(1024, 720);
    startTimerHz(30);
}

DesireAudioProcessorEditor::~DesireAudioProcessorEditor() = default;

void DesireAudioProcessorEditor::resized() {
    if (usingFallback_ && fallback_)
        fallback_->setBounds(getLocalBounds());
    else
        webView_.setBounds(getLocalBounds());
}

juce::RangedAudioParameter& DesireAudioProcessorEditor::parameter(const juce::ParameterID& id) const {
    auto* result = processor_.state().getParameter(id.getParamID());
    jassert(result != nullptr);
    return *result;
}

auto DesireAudioProcessorEditor::getResource(const juce::String& requestedUrl) const
    -> std::optional<Resource> {
    auto path = requestedUrl.upToFirstOccurrenceOf("?", false, false);
    path = path.fromFirstOccurrenceOf("/", false, false);

    if (path.isEmpty() || path == "Prototype/")
        path = "Prototype/index.html";

    if (path.contains(".."))
        return std::nullopt;

    auto bytes = resourceBytes(path);
    if (bytes.empty())
        return std::nullopt;

    return Resource { std::move(bytes), mimeTypeFor(path.fromLastOccurrenceOf(".", false, false)) };
}

juce::WebBrowserComponent::Options DesireAudioProcessorEditor::buildWebViewOptions() {
    auto options = juce::WebBrowserComponent::Options {};
#if JUCE_WINDOWS
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2 {}
                .withBackgroundColour(juce::Colour::fromRGB(5, 7, 11))
                .withUserDataFolder(
                    juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("BoratoDesireWebView2")));
#endif
    return options
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        .withInitialisationData("vendor", JucePlugin_Manufacturer)
        .withInitialisationData("pluginName", JucePlugin_Name)
        .withInitialisationData("pluginVersion", JucePlugin_VersionString)
        .withOptionsFrom(inputRelay_)
        .withOptionsFrom(bodyRelay_)
        .withOptionsFrom(heatRelay_)
        .withOptionsFrom(silkRelay_)
        .withOptionsFrom(desireRelay_)
        .withOptionsFrom(motionRelay_)
        .withOptionsFrom(widthRelay_)
        .withOptionsFrom(mixRelay_)
        .withOptionsFrom(outputRelay_)
        .withOptionsFrom(modeRelay_)
        .withOptionsFrom(bypassRelay_)
        .withNativeFunction("previousPreset", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.presetManager().selectPrevious();
            emitStatus();
            completion({});
        })
        .withNativeFunction("nextPreset", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.presetManager().selectNext();
            emitStatus();
            completion({});
        })
        .withNativeFunction("selectPreset", [this](const NativeArgs& args, NativeCompletion completion) {
            if (!args.isEmpty()) {
                const auto name = args[0].toString();
                const auto factoryNames = processor_.presetManager().factoryPresetNames();
                const int factoryIndex = factoryNames.indexOf(name);
                if (factoryIndex >= 0)
                    processor_.presetManager().selectFactory(factoryIndex);
                else
                    processor_.presetManager().selectUser(name);
            }
            emitStatus();
            completion({});
        })
        .withNativeFunction("saveUserPreset", [this](const NativeArgs& args, NativeCompletion completion) {
            if (!args.isEmpty())
                processor_.presetManager().saveUser(args[0].toString());
            emitStatus();
            completion({});
        })
        .withNativeFunction("deleteUserPreset", [this](const NativeArgs& args, NativeCompletion completion) {
            if (!args.isEmpty())
                processor_.presetManager().deleteUser(args[0].toString());
            emitStatus();
            completion({});
        })
        .withNativeFunction("selectAB", [this](const NativeArgs& args, NativeCompletion completion) {
            if (!args.isEmpty())
                processor_.selectABSlot(args[0].toString().equalsIgnoreCase("B") ? 'B' : 'A');
            emitStatus();
            completion({});
        })
        .withNativeFunction("copyAToB", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.copyAToB();
            emitStatus();
            completion({});
        })
        .withNativeFunction("copyBToA", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.copyBToA();
            emitStatus();
            completion({});
        })
        .withNativeFunction("undo", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.undo();
            emitStatus();
            completion({});
        })
        .withNativeFunction("redo", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.redo();
            emitStatus();
            completion({});
        })
        .withNativeFunction("setAnalyzerState", [this](const NativeArgs& args, NativeCompletion completion) {
            if (!args.isEmpty())
                applyAnalyzerStateVar(args[0]);
            completion(buildAnalyzerStateVar());
        })
        .withNativeFunction("getAnalyzerState", [this](const NativeArgs&, NativeCompletion completion) {
            completion(buildAnalyzerStateVar());
        })
        .withNativeFunction("revealPresetFolder", [this](const NativeArgs&, NativeCompletion completion) {
            processor_.presetManager().userPresetsFolder().revealToUser();
            completion({});
        })
        .withNativeFunction("openManual", [](const NativeArgs&, NativeCompletion completion) {
            completion({}); // ponytail: no manual shipped yet; wire a real URL/PDF when one exists
        })
        .withNativeFunction("reportUiReady", [this](const NativeArgs&, NativeCompletion completion) {
            webViewReady_ = true;
            emitStatus();
            completion({});
        })
        .withNativeFunction("reportJavaScriptError",
            [](const NativeArgs& args, NativeCompletion completion) {
                const juce::String message = args.size() > 0 ? args[0].toString() : juce::String();
                const juce::String file = args.size() > 1 ? args[1].toString() : juce::String();
                const int line = args.size() > 2 ? static_cast<int>(args[2]) : 0;
                juce::Logger::writeToLog("Desire UI error: " + message + " (" + file + ":"
                                          + juce::String(line) + ")");
                completion({});
            });
}

void DesireAudioProcessorEditor::showNativeFallback() {
    if (usingFallback_)
        return;
    usingFallback_ = true;
    webView_.setVisible(false);
    fallback_ = std::make_unique<NativeEditorFallback>(processor_.state());
    addAndMakeVisible(*fallback_);
    resized();
}

juce::var DesireAudioProcessorEditor::buildAnalyzerStateVar() const {
    auto tree = processor_.uiState();
    juce::DynamicObject::Ptr obj { new juce::DynamicObject() };
    obj->setProperty("view", tree.getProperty("view", "spectrum"));
    obj->setProperty("tap", tree.getProperty("tap", "post"));
    obj->setProperty("lowHz", tree.getProperty("lowHz", 20.0));
    obj->setProperty("highHz", tree.getProperty("highHz", 20000.0));
    obj->setProperty("reducedMotion", tree.getProperty("reducedMotion", false));
    return juce::var(obj.get());
}

void DesireAudioProcessorEditor::applyAnalyzerStateVar(const juce::var& value) {
    if (auto* obj = value.getDynamicObject()) {
        auto tree = processor_.uiState();
        for (const auto& key : { "view", "tap", "lowHz", "highHz", "reducedMotion" })
            if (obj->hasProperty(key))
                tree.setProperty(key, obj->getProperty(key), nullptr);
    }
    const auto tree = processor_.uiState();
    processor_.setAnalyzerEnabled(tree.getProperty("view", "spectrum").toString() != "off");
    processor_.setAnalyzerPostTap(tree.getProperty("tap", "post").toString() == "post");
}

void DesireAudioProcessorEditor::emitStatus() {
    juce::Array<juce::var> factoryArray;
    for (const auto& name : processor_.presetManager().factoryPresetNames())
        factoryArray.add(name);
    juce::Array<juce::var> userArray;
    for (const auto& name : processor_.presetManager().userPresetNames())
        userArray.add(name);

    juce::DynamicObject::Ptr obj { new juce::DynamicObject() };
    obj->setProperty("presetName", processor_.presetManager().currentPresetName());
    obj->setProperty("factoryPresets", factoryArray);
    obj->setProperty("userPresets", userArray);
    obj->setProperty("abSlot", juce::String::charToString(processor_.currentABSlot()));
    obj->setProperty("canUndo", processor_.undoManager().canUndo());
    obj->setProperty("canRedo", processor_.undoManager().canRedo());
    webView_.emitEventIfBrowserIsVisible("desireStatus", juce::var(obj.get()));
}

void DesireAudioProcessorEditor::emitVisualFrame() {
    const auto peaks = processor_.meterLevels();
    const auto rms = processor_.rmsLevels();

    if (processor_.spectrumAnalyser().isNextBlockReady())
        processor_.spectrumAnalyser().computeBins(spectrumBins_, bandEnergy_);

    juce::DynamicObject::Ptr input { new juce::DynamicObject() };
    input->setProperty("peak", juce::Array<juce::var> { peaks[0], peaks[1] });
    input->setProperty("rms", juce::Array<juce::var> { rms[0], rms[1] });

    juce::DynamicObject::Ptr output { new juce::DynamicObject() };
    output->setProperty("peak", juce::Array<juce::var> { peaks[2], peaks[3] });
    output->setProperty("rms", juce::Array<juce::var> { rms[2], rms[3] });

    juce::DynamicObject::Ptr energy { new juce::DynamicObject() };
    energy->setProperty("low", bandEnergy_[0]);
    energy->setProperty("mid", bandEnergy_[1]);
    energy->setProperty("high", bandEnergy_[2]);

    juce::Array<juce::var> spectrum;
    spectrum.ensureStorageAllocated(static_cast<int>(spectrumBins_.size()));
    for (float bin : spectrumBins_)
        spectrum.add(bin);

    juce::DynamicObject::Ptr frame { new juce::DynamicObject() };
    frame->setProperty("sequence", static_cast<double>(++frameSequence_));
    frame->setProperty("input", juce::var(input.get()));
    frame->setProperty("output", juce::var(output.get()));
    frame->setProperty("energy", juce::var(energy.get()));
    frame->setProperty("gainReduction", processor_.gainReductionDb() / 24.0f);
    frame->setProperty("desireEnergy", processor_.desireEnergy());
    frame->setProperty("spectrum", spectrum);

    webView_.emitEventIfBrowserIsVisible("desireVisualFrame", juce::var(frame.get()));
}

void DesireAudioProcessorEditor::timerCallback() {
    emitVisualFrame();

#if defined(DESIRE_UI_BACKEND_AUTO)
    if (!usingFallback_ && !webViewReady_
        && juce::Time::getMillisecondCounter() - startTimeMs_ > 3000)
        showNativeFallback();
#endif
}

} // namespace desire
