(function () {
    "use strict";

    const sliderNames = [
        "input", "body", "heat", "silk", "desire",
        "motion", "width", "mix", "output"
    ];
    const modeNames = ["intimate", "club", "after-dark"];

    function createMockSliderState(initial) {
        const listeners = [];
        let value = initial;
        return {
            getNormalisedValue: () => value,
            setNormalisedValue: (v) => { value = v; listeners.forEach((fn) => fn()); },
            sliderDragStarted: () => {},
            sliderDragEnded: () => {},
            valueChangedEvent: { addListener: (fn) => listeners.push(fn) },
            propertiesChangedEvent: { addListener: () => {} }
        };
    }

    function createMockToggleState(initial) {
        let value = initial;
        return {
            getValue: () => value,
            setValue: (v) => { value = v; }
        };
    }

    function showBrowserPreviewBadge() {
        if (document.getElementById("desire-preview-badge")) return;
        const badge = document.createElement("div");
        badge.id = "desire-preview-badge";
        badge.textContent = "BROWSER PREVIEW";
        badge.setAttribute("aria-hidden", "true");
        document.body.appendChild(badge);
    }

    function installErrorOverlay(reportError) {
        let overlay = null;
        function ensureOverlay() {
            if (overlay) return overlay;
            overlay = document.createElement("div");
            overlay.id = "desire-error-overlay";
            document.body.appendChild(overlay);
            return overlay;
        }
        function show(message) {
            const el = ensureOverlay();
            const line = document.createElement("div");
            line.textContent = message;
            el.appendChild(line);
            el.style.display = "block";
        }
        window.addEventListener("error", (event) => {
            const message = event.message || "Unknown error";
            const file = event.filename || "";
            const lineNumber = event.lineno || 0;
            show(message + (file ? " (" + file + ":" + lineNumber + ")" : ""));
            reportError(message, file, lineNumber);
        });
        window.addEventListener("unhandledrejection", (event) => {
            const message = "Unhandled rejection: " + (event.reason && event.reason.message
                ? event.reason.message : String(event.reason));
            show(message);
            reportError(message, "", 0);
        });
    }

    async function connect() {
        const hasNativeBackend = Boolean(window.__JUCE__ && window.__JUCE__.backend);

        let juceModule = null;
        if (hasNativeBackend) {
            juceModule = await import("./juce/index.js");
        } else {
            showBrowserPreviewBadge();
        }

        const sliders = new Map();
        sliderNames.forEach((name) => {
            const state = hasNativeBackend
                ? juceModule.getSliderState(name)
                : createMockSliderState(parseFloat(
                    document.querySelector('[data-param="' + name + '"]')?.dataset.value || "0.5"));
            sliders.set(name, state);
            const refresh = () => {
                if (window.DesireKnobs) window.DesireKnobs.setValue(name, state.getNormalisedValue());
            };
            state.valueChangedEvent.addListener(refresh);
            state.propertiesChangedEvent.addListener(refresh);
        });

        const mode = hasNativeBackend ? juceModule.getComboBoxState("mode") : {
            listeners: [],
            index: 1,
            getChoiceIndex() { return this.index; },
            setChoiceIndex(i) { this.index = i; this.listeners.forEach((fn) => fn()); },
            valueChangedEvent: { addListener(fn) { this.listeners = this.listeners || []; this.listeners.push(fn); } },
            propertiesChangedEvent: { addListener() {} }
        };
        const refreshMode = () => {
            if (window.DesireModes) window.DesireModes.setMode(modeNames[mode.getChoiceIndex()] || "club");
        };
        mode.valueChangedEvent.addListener(refreshMode);
        mode.propertiesChangedEvent.addListener(refreshMode);

        const bypass = hasNativeBackend ? juceModule.getToggleState("bypass") : createMockToggleState(false);

        const callNative = (name, ...args) =>
            hasNativeBackend ? juceModule.getNativeFunction(name)(...args) : Promise.resolve(undefined);

        window.DesireBridge = {
            isNativeBackend: hasNativeBackend,
            getSliderState: (id) => sliders.get(id),
            getToggleState: () => bypass,
            getChoiceState: () => mode,
            callNative,
            getInitialisationData: (key) =>
                hasNativeBackend ? window.__JUCE__.initialisationData[key] : undefined,
            on(eventName, callback) {
                if (hasNativeBackend) window.__JUCE__.backend.addEventListener(eventName, callback);
            }
        };

        installErrorOverlay((message, file, line) => callNative("reportJavaScriptError", message, file, line));

        const refreshAllControls = () => {
            sliders.forEach((state, name) => {
                if (window.DesireKnobs) window.DesireKnobs.setValue(name, state.getNormalisedValue());
            });
            refreshMode();
        };
        if (document.readyState === "loading")
            document.addEventListener("DOMContentLoaded", refreshAllControls, { once: true });
        else
            refreshAllControls();

        document.addEventListener("desire:knobgesturestart", (event) => {
            sliders.get(event.detail.param)?.sliderDragStarted();
        });
        document.addEventListener("desire:knobchange", (event) => {
            sliders.get(event.detail.param)?.setNormalisedValue(event.detail.value);
        });
        document.addEventListener("desire:knobgestureend", (event) => {
            sliders.get(event.detail.param)?.sliderDragEnded();
        });
        document.addEventListener("desire:modechange", (event) => {
            const index = modeNames.indexOf(event.detail.mode);
            if (index >= 0) mode.setChoiceIndex(index);
        });
        document.addEventListener("desire:bypasschange", (event) => {
            bypass.setValue(Boolean(event.detail.bypassed));
        });
        document.addEventListener("desire:presetcommand", (event) => {
            const { command, arg } = event.detail;
            callNative(command, arg);
        });
        document.addEventListener("desire:analyzercommand", (event) => {
            callNative("setAnalyzerState", event.detail);
        });

        window.DesireBridge.on("desireVisualFrame", (frame) => {
            document.dispatchEvent(new CustomEvent("desire:visualframe", { detail: frame }));
            document.dispatchEvent(new CustomEvent("desire:levels", {
                detail: {
                    inputL: frame.input.peak[0], inputR: frame.input.peak[1],
                    outputL: frame.output.peak[0], outputR: frame.output.peak[1]
                }
            }));
        });
        window.DesireBridge.on("desireStatus", (status) => {
            document.dispatchEvent(new CustomEvent("desire:status", { detail: status }));
        });

        if (hasNativeBackend) callNative("reportUiReady");
    }

    connect().catch((error) => console.error("Desire/JUCE bridge failed", error));
})();
