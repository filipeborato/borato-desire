// Zero-dependency graphics-layer validation: run with `node --test`.
// Exercises the pure mapping functions shared by knobs.js/meters.js/
// analyzer-controls.js (Prototype/scripts/mapping.js) -- no DOM, no browser.
"use strict";

const test = require("node:test");
const assert = require("node:assert/strict");
const DesireMapping = require("../Prototype/scripts/mapping.js");

test("knobValueToAngleDeg maps 0..1 onto -135..135 and clamps", () => {
    assert.equal(DesireMapping.knobValueToAngleDeg(0), -135);
    assert.equal(DesireMapping.knobValueToAngleDeg(1), 135);
    assert.equal(DesireMapping.knobValueToAngleDeg(0.5), 0);
    assert.equal(DesireMapping.knobValueToAngleDeg(-5), -135);
    assert.equal(DesireMapping.knobValueToAngleDeg(5), 135);
});

test("knobValueToDb maps 0..1 onto -24..+12 dB", () => {
    assert.equal(DesireMapping.knobValueToDb(0), -24);
    assert.equal(DesireMapping.knobValueToDb(1), 12);
    assert.ok(Math.abs(DesireMapping.knobValueToDb(0.5) - -6) < 1e-9);
});

test("knobValueToWidthPercent maps 0..1 onto 0..200", () => {
    assert.equal(DesireMapping.knobValueToWidthPercent(0), 0);
    assert.equal(DesireMapping.knobValueToWidthPercent(1), 200);
    assert.equal(DesireMapping.knobValueToWidthPercent(0.5), 100);
});

test("knobValueToPercent maps 0..1 onto 0..100", () => {
    assert.equal(DesireMapping.knobValueToPercent(0), 0);
    assert.equal(DesireMapping.knobValueToPercent(1), 100);
});

test("arcStrokeDashArray covers 270 degrees of the circle at full value", () => {
    const radius = 48;
    const circumference = 2 * Math.PI * radius;

    const zero = DesireMapping.arcStrokeDashArray(0, radius);
    assert.equal(zero.current, 0);
    assert.ok(Math.abs(zero.circumference - circumference) < 1e-9);

    const full = DesireMapping.arcStrokeDashArray(1, radius);
    assert.ok(Math.abs(full.current - circumference * 0.75) < 1e-9);

    const half = DesireMapping.arcStrokeDashArray(0.5, radius);
    assert.ok(Math.abs(half.current - full.current * 0.5) < 1e-9);
});

test("meterBallisticStep rises instantly and falls exponentially", () => {
    assert.equal(DesireMapping.meterBallisticStep(0.8, 0.2, 0.88), 0.8);
    assert.ok(Math.abs(DesireMapping.meterBallisticStep(0.0, 0.5, 0.88) - 0.44) < 1e-9);
    // Never falls below the new target even if decay would undershoot it.
    assert.equal(DesireMapping.meterBallisticStep(0.5, 0.5, 0.88), 0.5);
});

test("analyzer Hz<->normalized mapping is monotonic and round-trips", () => {
    assert.equal(DesireMapping.analyzerNormalizedToHz(0), 20);
    assert.ok(Math.abs(DesireMapping.analyzerNormalizedToHz(1) - 20000) < 1e-6);

    for (const normalized of [0, 0.25, 0.5, 0.75, 1]) {
        const hz = DesireMapping.analyzerNormalizedToHz(normalized);
        const roundTripped = DesireMapping.analyzerHzToNormalized(hz);
        assert.ok(Math.abs(roundTripped - normalized) < 1e-9);
    }

    const low = DesireMapping.analyzerNormalizedToHz(0.2);
    const high = DesireMapping.analyzerNormalizedToHz(0.8);
    assert.ok(high > low);
});

test("analyzerFormatFrequency switches units at 1 kHz", () => {
    assert.equal(DesireMapping.analyzerFormatFrequency(999), "999.0 Hz");
    assert.equal(DesireMapping.analyzerFormatFrequency(1500), "1.5 kHz");
});

test("meterLevelToHeight uses a dB scale, not raw linear amplitude", () => {
    // The reported bug: a linear meter showed a clipping (>=0dBFS) signal only
    // "halfway up" because 0dBFS in dB terms is amplitude 1.0, but -6dBFS
    // (already hot) is amplitude 0.5 -- which a linear scaleY() draws as 50%.
    assert.equal(DesireMapping.meterLevelToHeight(0), 0);
    assert.equal(DesireMapping.meterLevelToHeight(1.0), 1); // 0 dBFS -> full height
    assert.equal(DesireMapping.meterLevelToHeight(2.0), 1); // overs still clamp to full, not overflow

    // -6dBFS (amplitude 0.5) is loud and close to clipping; a dB-scaled meter
    // must show it much closer to full than a linear one (which would show 0.5).
    const minusSixDb = DesireMapping.meterLevelToHeight(0.5);
    assert.ok(minusSixDb > 0.7, `-6dBFS should read well above half-height, got ${minusSixDb}`);

    // Monotonic across the range.
    const quiet = DesireMapping.meterLevelToHeight(0.01);
    const mid = DesireMapping.meterLevelToHeight(0.1);
    const loud = DesireMapping.meterLevelToHeight(0.9);
    assert.ok(quiet < mid && mid < loud && loud < 1);
});

test("responseCurveY rises near 220Hz as Body increases, and stays flat when Body=0", () => {
    const flat = DesireMapping.responseCurveY(220, 0, 0);
    const boosted = DesireMapping.responseCurveY(220, 1, 0);
    // SVG y grows downward: a boost must produce a *smaller* y (curve rises).
    assert.ok(boosted < flat, `boosted (${boosted}) should be above flat (${flat})`);

    // Far from 220Hz, Body's effect should be much smaller than right at its
    // centre frequency (bell-shaped/local, not a global broadband boost).
    const flatFar = DesireMapping.responseCurveY(20, 0, 0);
    const boostedFar = DesireMapping.responseCurveY(20, 1, 0);
    const effectAt220 = flat - boosted;
    const effectAt20 = flatFar - boostedFar;
    assert.ok(effectAt20 < effectAt220 * 0.25,
        `effect at 20Hz (${effectAt20}) should be well below effect at 220Hz (${effectAt220})`);
});

test("silkCutoffHz moves down (more rolloff) as Silk increases, and responseCurveY drops above it", () => {
    assert.equal(DesireMapping.silkCutoffHz(0), 20000);
    assert.equal(DesireMapping.silkCutoffHz(1), 5500);
    assert.ok(DesireMapping.silkCutoffHz(0.5) < DesireMapping.silkCutoffHz(0.2));

    const flat = DesireMapping.responseCurveY(15000, 0, 0);
    const rolledOff = DesireMapping.responseCurveY(15000, 0, 1);
    // SVG y grows downward: rolloff must produce a *larger* y (curve dips).
    assert.ok(rolledOff > flat, `rolled-off (${rolledOff}) should be below flat (${flat})`);
});

test("DEFAULT_NORMALISED matches the 'Late Night Confessions' starting point, not 50%", () => {
    const defaults = DesireMapping.DEFAULT_NORMALISED;
    for (const param of ["input", "body", "heat", "silk", "desire", "motion", "width", "mix", "output"])
        assert.ok(Number.isFinite(defaults[param]), `${param} should have a finite default`);

    // Double-click-to-reset must land back on 0 dB, not the middle of the range.
    // Input defaults to -10dB headroom (not 0dB) so hot/mastered material
    // doesn't hit the drive stage at full level by default; Output stays at 0dB.
    assert.ok(Math.abs(DesireMapping.knobValueToDb(defaults.input) - -10) < 1e-9);
    assert.ok(Math.abs(DesireMapping.knobValueToDb(defaults.output) - 0) < 1e-9);
    assert.ok(Math.abs(DesireMapping.knobValueToWidthPercent(defaults.width) - 128) < 1e-9);
    assert.notEqual(defaults.desire, 0.5);
});
