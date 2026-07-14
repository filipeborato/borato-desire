# Desire Spectrum Visualizer

- Bounds: 0, 0, 1156, 540
- Render Method: `juce::Path` generated from audio buffer FFT magnitudes.
- Geometry: A continuous filled mountain-style path from `x=0` to `x=1156`.
- Fill: `juce::ColourGradient` from `DesireColours::pink` (opacity 0.5) at the peaks to Transparent at the bottom (y=540).
- Stroke (Top Outline): `DesireColours::pink`, thickness 2.0f.
- Target: `DesireDisplay::paintSpectrum()`
