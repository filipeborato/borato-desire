// Pure, DOM-free mapping functions shared by the knob/meter/analyzer UI and by
// the Node test suite (tests/graphics-mapping.test.js). No framework, no DOM:
// keeping this file side-effect-free is what makes it testable with `node --test`
// without a browser.
(function (root) {
    "use strict";

    const MIN_ANGLE = -135;
    const MAX_ANGLE = 135;
    const ARC_SWEEP_DEG = 270;
    const DB_MIN = -24;
    const DB_MAX = 12;
    const WIDTH_MAX_PERCENT = 200;
    const HZ_MIN = 20;
    const HZ_MAX = 20000;

    // Normalised (0..1) parameter defaults, matching DesireAudioProcessor's
    // createParameterLayout() -- the "Late Night Confessions" starting point.
    // Double-clicking a knob restores this, not a blanket 50%.
    const DEFAULT_NORMALISED = {
        input: 14 / 36,     // -10 dB on a -24..+12 dB range
        body: 0.65,
        heat: 0.72,
        silk: 0.58,
        desire: 0.76,
        motion: 0.64,
        width: 0.64,        // 128% on a 0..200% range
        mix: 0.42,
        output: 24 / 36     // 0 dB
    };

    function clamp01(value) {
        return Math.max(0, Math.min(1, value));
    }

    function knobValueToAngleDeg(normalized) {
        const value = clamp01(normalized);
        return MIN_ANGLE + value * (MAX_ANGLE - MIN_ANGLE);
    }

    function knobValueToDb(normalized) {
        const value = clamp01(normalized);
        return DB_MIN + value * (DB_MAX - DB_MIN);
    }

    function knobValueToWidthPercent(normalized) {
        return clamp01(normalized) * WIDTH_MAX_PERCENT;
    }

    function knobValueToPercent(normalized) {
        return clamp01(normalized) * 100;
    }

    function arcStrokeDashArray(normalized, radius) {
        const value = clamp01(normalized);
        const circumference = 2 * Math.PI * radius;
        const totalArcLength = circumference * (ARC_SWEEP_DEG / 360);
        return { current: totalArcLength * value, circumference };
    }

    // Ballistic meter decay: rises instantly to the target, falls exponentially.
    function meterBallisticStep(target, previous, decayFactor) {
        return target >= previous ? target : Math.max(target, previous * decayFactor);
    }

    const METER_MIN_DB = -48;

    // Linear peak amplitude -> 0..1 bar height on a dB scale. Using the raw
    // linear amplitude directly (amplitude === scaleY) made loud/clipping
    // signals look like they were only "halfway up": -6dBFS is amplitude 0.5
    // (already hot, close to 0dBFS) but a linear meter draws that as 50%.
    // A dB scale is what every real peak/VU meter uses, and what makes a bar
    // that's actually near clipping actually look near-full.
    function meterLevelToHeight(linearAmplitude) {
        if (!(linearAmplitude > 0)) return 0;
        const db = 20 * Math.log10(linearAmplitude);
        return clamp01((db - METER_MIN_DB) / (0 - METER_MIN_DB));
    }

    function analyzerNormalizedToHz(normalized) {
        return HZ_MIN * Math.pow(HZ_MAX / HZ_MIN, clamp01(normalized));
    }

    function analyzerHzToNormalized(hz) {
        const logRange = Math.log(HZ_MAX / HZ_MIN);
        return Math.log(Math.max(HZ_MIN, hz) / HZ_MIN) / logRange;
    }

    function analyzerFormatFrequency(hz) {
        if (hz >= 1000) return (hz / 1000).toFixed(1) + " kHz";
        return hz.toFixed(1) + " Hz";
    }

    // Response curve: a JS-side visual approximation of DesireEngine's two tone
    // stages (Body low-shelf around 220Hz, Silk lowpass), NOT a query into the
    // real filter coefficients -- it exists so the curve reacts to Body/Silk
    // like the mock always implied it eventually would ("no plugin ela será
    // alimentada pela resposta DSP"), not to be a literal frequency-response plot.
    const CURVE_BASELINE_Y = 400;
    const CURVE_BODY_FREQ_HZ = 220;
    const CURVE_BODY_MAX_RISE = 110;
    const CURVE_BODY_BANDWIDTH_OCTAVES = 1.6;
    const CURVE_SILK_MAX_DROP = 90;
    const CURVE_SILK_MIN_CUTOFF_HZ = 5500;   // mirrors DesireEngine::kSilkMinHz
    const CURVE_SILK_MAX_CUTOFF_HZ = 20000;  // mirrors DesireEngine::kSilkMaxHz

    function curveBodyGain(freqHz, bodyAmount) {
        const octavesFromCentre = Math.log(freqHz / CURVE_BODY_FREQ_HZ) / Math.LN2;
        return clamp01(bodyAmount) * CURVE_BODY_MAX_RISE
            * Math.exp(-(octavesFromCentre * octavesFromCentre) / (2 * CURVE_BODY_BANDWIDTH_OCTAVES * CURVE_BODY_BANDWIDTH_OCTAVES));
    }

    // Higher Silk = more high-frequency rolloff (it tames saturation harshness,
    // it isn't an "add brightness" control) -- same direction as the engine.
    function silkCutoffHz(silkAmount) {
        return CURVE_SILK_MAX_CUTOFF_HZ
            - clamp01(silkAmount) * (CURVE_SILK_MAX_CUTOFF_HZ - CURVE_SILK_MIN_CUTOFF_HZ);
    }

    function curveSilkLoss(freqHz, silkAmount) {
        const cutoff = silkCutoffHz(silkAmount);
        const octavesAboveCutoff = Math.log(freqHz / cutoff) / Math.LN2;
        const shape = 1 / (1 + Math.exp(-octavesAboveCutoff / 0.45));
        return clamp01(silkAmount) * CURVE_SILK_MAX_DROP * shape;
    }

    function responseCurveY(freqHz, bodyAmount, silkAmount) {
        return CURVE_BASELINE_Y - curveBodyGain(freqHz, bodyAmount) + curveSilkLoss(freqHz, silkAmount);
    }

    const DesireMapping = {
        DEFAULT_NORMALISED,
        knobValueToAngleDeg,
        knobValueToDb,
        knobValueToWidthPercent,
        knobValueToPercent,
        arcStrokeDashArray,
        meterBallisticStep,
        meterLevelToHeight,
        analyzerNormalizedToHz,
        analyzerHzToNormalized,
        analyzerFormatFrequency,
        CURVE_BODY_FREQ_HZ,
        silkCutoffHz,
        responseCurveY
    };

    if (typeof module !== "undefined" && module.exports) {
        module.exports = DesireMapping;
    } else {
        root.DesireMapping = DesireMapping;
    }
})(typeof window !== "undefined" ? window : globalThis);
