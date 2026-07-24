document.addEventListener("DOMContentLoaded", () => {
    const svg = document.getElementById("dynamic-display");
    const spectrumLayer = document.getElementById("spectrum-layer");
    const curveLayer = document.getElementById("curve-layer");
    const particleLayer = document.getElementById("particle-layer");

    if (!svg || !spectrumLayer || !curveLayer || !particleLayer) {
        return;
    }

    const SVG_NS = "http://www.w3.org/2000/svg";
    const WIDTH = 1156;
    const BASELINE = 470;
    const BIN_COUNT = 192;
    const hasNativeAudioBackend = Boolean(window.__JUCE__ && window.__JUCE__.backend);
    const analyzerState = {
        visible: true,
        dataAvailable: true,
        lowNormalized: 0,
        highNormalized: 1
    };
    let realSpectrumBins = null;

    if (hasNativeAudioBackend) {
        document.addEventListener("desire:visualframe", (event) => {
            const frame = event.detail || {};
            if (Array.isArray(frame.spectrum) && frame.spectrum.length > 0)
                realSpectrumBins = frame.spectrum;
            if (frame.desireEnergy !== undefined)
                document.documentElement.style.setProperty("--desire-energy", frame.desireEnergy);
        });
    }

    const defs = svg.querySelector("defs");
    defs.insertAdjacentHTML("beforeend",
        '<linearGradient id="spectrumGradient" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="' + WIDTH + '" y2="0">' +
            '<stop offset="0" stop-color="#ff318f" />' +
            '<stop offset="0.48" stop-color="#f02c9f" />' +
            '<stop offset="0.63" stop-color="#8a55d9" />' +
            '<stop offset="1" stop-color="#24d8ff" />' +
        '</linearGradient>' +
        '<linearGradient id="curveGradient" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="' + WIDTH + '" y2="0">' +
            '<stop offset="0" stop-color="#ff8cc8" />' +
            '<stop offset="0.52" stop-color="#ff318f" />' +
            '<stop offset="1" stop-color="#b825a7" />' +
        '</linearGradient>');

    function createPath(parent, attributes) {
        const path = document.createElementNS(SVG_NS, "path");
        Object.entries(attributes).forEach(([name, value]) => path.setAttribute(name, value));
        parent.appendChild(path);
        return path;
    }

    const spectrumGlow = createPath(spectrumLayer, {
        fill: "none",
        stroke: "url(#spectrumGradient)",
        "stroke-width": "4",
        opacity: "0.10"
    });
    spectrumGlow.style.filter = "blur(2px)";

    const spectrumPath = createPath(spectrumLayer, {
        fill: "none",
        stroke: "url(#spectrumGradient)",
        "stroke-width": "1.2",
        opacity: "0.62"
    });

    // Response curve: reacts to Body and Silk (DesireEngine's two tone stages),
    // with two draggable nodes that ARE Body and Silk -- dragging one is the
    // same as turning that knob (same gesture events knobs.js already dispatches,
    // consumed by the same bridge.js listeners; no separate control path).
    const curveGlow = createPath(curveLayer, {
        fill: "none",
        stroke: "#ff318f",
        "stroke-width": "12",
        "stroke-linecap": "round",
        opacity: "0.18"
    });
    curveGlow.style.filter = "blur(5px)";

    const curveSolid = createPath(curveLayer, {
        fill: "none",
        stroke: "url(#curveGradient)",
        "stroke-width": "3.2",
        "stroke-linecap": "round"
    });

    const curveParams = { body: 0.65, silk: 0.58 }; // "Late Night Confessions" defaults

    function createCurveNode(param) {
        const group = document.createElementNS(SVG_NS, "g");
        group.setAttribute("class", "curve-node");
        group.setAttribute("tabindex", "0");
        group.setAttribute("role", "slider");
        group.setAttribute("aria-label", param === "body" ? "Body" : "Silk");
        group.innerHTML =
            '<circle r="8" fill="#10111a" stroke="#ff5ca9" stroke-width="2.2" />' +
            '<circle r="2.7" fill="#fff" />';
        group.style.filter = "drop-shadow(0 0 5px rgba(255, 49, 143, 0.75))";
        group.style.cursor = "ns-resize";
        group.style.pointerEvents = "auto";
        group.style.touchAction = "none";
        curveLayer.appendChild(group);

        let dragStartY = 0;
        let dragStartValue = 0;
        group.addEventListener("pointerdown", (event) => {
            dragStartY = event.clientY;
            dragStartValue = curveParams[param];
            group.setPointerCapture(event.pointerId);
            group.dispatchEvent(new CustomEvent("desire:knobgesturestart", { bubbles: true, detail: { param } }));
        });
        group.addEventListener("pointermove", (event) => {
            if (!group.hasPointerCapture(event.pointerId)) return;
            const deltaY = dragStartY - event.clientY;
            const value = Math.max(0, Math.min(1, dragStartValue + deltaY / 250));
            group.dispatchEvent(new CustomEvent("desire:knobchange", { bubbles: true, detail: { param, value } }));
        });
        group.addEventListener("pointerup", (event) => {
            if (!group.hasPointerCapture(event.pointerId)) return;
            group.releasePointerCapture(event.pointerId);
            group.dispatchEvent(new CustomEvent("desire:knobgestureend", { bubbles: true, detail: { param } }));
        });
        group.addEventListener("dblclick", () => {
            const value = DesireMapping.DEFAULT_NORMALISED[param];
            if (value === undefined) return;
            group.dispatchEvent(new CustomEvent("desire:knobgesturestart", { bubbles: true, detail: { param } }));
            group.dispatchEvent(new CustomEvent("desire:knobchange", { bubbles: true, detail: { param, value } }));
            group.dispatchEvent(new CustomEvent("desire:knobgestureend", { bubbles: true, detail: { param } }));
        });

        return group;
    }

    const bodyNode = createCurveNode("body");
    const silkNode = createCurveNode("silk");

    function freqToX(freqHz) {
        return DesireMapping.analyzerHzToNormalized(freqHz) * WIDTH;
    }

    function renderCurve() {
        const { body, silk } = curveParams;
        const steps = 48;
        let d = "";
        for (let i = 0; i <= steps; i += 1) {
            const t = i / steps;
            const freq = DesireMapping.analyzerNormalizedToHz(t);
            const x = t * WIDTH;
            const y = DesireMapping.responseCurveY(freq, body, silk);
            d += (i === 0 ? "M " : "L ") + x.toFixed(1) + " " + y.toFixed(1) + " ";
        }
        curveGlow.setAttribute("d", d);
        curveSolid.setAttribute("d", d);

        const bodyFreq = DesireMapping.CURVE_BODY_FREQ_HZ;
        bodyNode.setAttribute("transform",
            "translate(" + freqToX(bodyFreq).toFixed(1) + " " + DesireMapping.responseCurveY(bodyFreq, body, silk).toFixed(1) + ")");

        const silkFreq = DesireMapping.silkCutoffHz(silk);
        silkNode.setAttribute("transform",
            "translate(" + freqToX(silkFreq).toFixed(1) + " " + DesireMapping.responseCurveY(silkFreq, body, silk).toFixed(1) + ")");
    }

    document.addEventListener("desire:knobvisualupdate", (event) => {
        const { param, value } = event.detail || {};
        if (param !== "body" && param !== "silk") return;
        curveParams[param] = value;
        renderCurve();
    });

    renderCurve();

    // Pontos discretos, criados uma vez. Sem alocação ou remoção por frame.
    for (let i = 0; i < 28; i += 1) {
        const point = document.createElementNS(SVG_NS, "circle");
        const seed = hash(i + 41);
        point.setAttribute("cx", (205 + seed * 740).toFixed(1));
        point.setAttribute("cy", (190 + hash(i + 79) * 245).toFixed(1));
        point.setAttribute("r", (0.6 + hash(i + 113) * 1.5).toFixed(1));
        point.setAttribute("fill", i % 5 === 0 ? "#24d8ff" : "#ff318f");
        point.setAttribute("opacity", (0.18 + hash(i + 151) * 0.38).toFixed(2));
        particleLayer.appendChild(point);
    }

    function hash(value) {
        const x = Math.sin(value * 12.9898) * 43758.5453;
        return x - Math.floor(x);
    }

    function gaussian(value, centre, spread) {
        const distance = (value - centre) / spread;
        return Math.exp(-0.5 * distance * distance);
    }

    function spectrumHeight(index, seconds) {
        const t = index / (BIN_COUNT - 1);
        const envelope =
            10 +
            68 * gaussian(t, 0.10, 0.055) +
            118 * gaussian(t, 0.32, 0.085) +
            76 * gaussian(t, 0.51, 0.075) +
            48 * gaussian(t, 0.67, 0.11) +
            20 * gaussian(t, 0.84, 0.12);
        const binTexture = 0.30 + 0.70 * hash(index + 7);
        const slowMotion = 0.94 + 0.06 * Math.sin(seconds * 1.45 + index * 0.19);
        return Math.max(2, envelope * binTexture * slowMotion);
    }

    function renderSpectrumFromBins(bins) {
        const binWidth = WIDTH / bins.length;
        let path = "";

        for (let i = 0; i < bins.length; i += 1) {
            const normalized = i / (bins.length - 1);
            if (normalized < analyzerState.lowNormalized || normalized > analyzerState.highNormalized) {
                continue;
            }

            const x = (i + 0.5) * binWidth;
            const top = BASELINE - Math.max(2, bins[i] * 210);
            path += "M " + x.toFixed(1) + " " + BASELINE + " V " + top.toFixed(1) + " ";
        }

        spectrumGlow.setAttribute("d", path);
        spectrumPath.setAttribute("d", path);
    }

    function renderSpectrum(seconds) {
        if (hasNativeAudioBackend && realSpectrumBins) {
            renderSpectrumFromBins(realSpectrumBins);
            return;
        }

        const binWidth = WIDTH / BIN_COUNT;
        let path = "";

        for (let i = 0; i < BIN_COUNT; i += 1) {
            const normalized = i / (BIN_COUNT - 1);
            if (normalized < analyzerState.lowNormalized || normalized > analyzerState.highNormalized) {
                continue;
            }

            const x = (i + 0.5) * binWidth;
            const top = BASELINE - spectrumHeight(i, seconds);
            path += "M " + x.toFixed(1) + " " + BASELINE + " V " + top.toFixed(1) + " ";
        }

        spectrumGlow.setAttribute("d", path);
        spectrumPath.setAttribute("d", path);
    }

    document.addEventListener("desire:analyzerchange", (event) => {
        const detail = event.detail;
        const logRange = Math.log(20000 / 20);
        analyzerState.visible = detail.view === "spectrum";
        analyzerState.lowNormalized = Math.log(detail.lowHz / 20) / logRange;
        analyzerState.highNormalized = Math.log(detail.highHz / 20) / logRange;
        spectrumLayer.style.display = analyzerState.visible && analyzerState.dataAvailable ? "" : "none";
        if (analyzerState.dataAvailable) renderSpectrum(performance.now() / 1000);
    });

    const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    let lastFrame = -Infinity;

    function animate(timestamp) {
        if (timestamp - lastFrame >= 33) {
            renderSpectrum(timestamp / 1000);
            lastFrame = timestamp;
        }
        requestAnimationFrame(animate);
    }

    if (analyzerState.dataAvailable) renderSpectrum(0);
    else spectrumLayer.style.display = "none";
    if (analyzerState.dataAvailable && !reduceMotion) {
        requestAnimationFrame(animate);
    }
});
