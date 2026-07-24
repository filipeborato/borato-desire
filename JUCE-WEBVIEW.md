# Borato Desire: JUCE + WebView

The web prototype remains the UI source. CMake stages `Prototype`, the required
`Design` assets, the canonical dancer from `Assets`, and JUCE's JavaScript bridge,
then embeds the resulting zip in the plugin binary.

## Configure and build on this machine

```powershell
cmake --preset windows-vs2026-picotado-juce
cmake --build --preset standalone-debug
```

The standalone application is generated at:

```text
build/plugin/BoratoDesire_artefacts/Debug/Standalone/Borato Desire.exe
```

Without the Picotado checkout, use `cmake --preset windows-vs2026`; JUCE 8.0.12
will be acquired into `libs/juce` with CPM.

## Integration boundary

- Knobs, mode, and bypass are APVTS parameters synchronized by JUCE web relays
  (`WebSliderRelay`/`WebComboBoxRelay`/`WebToggleButtonRelay` + their
  `...ParameterAttachment` counterparts) — the web page never holds a second
  source of truth for a parameter value.
- Browser events, resource loading, and all `withNativeFunction` handlers
  (preset nav, A/B, undo/redo, analyzer state) run on the message thread.
- The audio callback never accesses the browser, DOM, or filesystem. Meters
  cross the boundary through atomics; the spectrum FFT runs on the message
  thread off a lock-free single-flag handoff from `SpectrumAnalyser`. Both are
  combined into one `desireVisualFrame` event, emitted at 30 Hz.
- `DesireEngine` (`Source/DesireEngine.h/.cpp`) is the real DSP: input trim →
  body shelf → drive/waveshape (heat + desire + motion LFO) → silk lowpass →
  mid/side width → dry/wet mix (bypass collapses mix to 0) → output trim.
  Validated by `tests/DesireEngineTests.cpp` (`ctest`).
- `BORATO_DESIRE_UI_BACKEND=NATIVE` skips the WebView entirely in favour of
  `NativeEditorFallback`; `AUTO` starts the WebView and swaps to that fallback
  if `reportUiReady` hasn't fired within 3s of construction. `WEBVIEW` (the
  default) never falls back.
