# Borato Desire v0.0.1

The first build of Borato Desire: a saturation/character plugin with a full
WebView2-hosted interface, a from-scratch DSP engine, presets, A/B, undo/redo,
and a CI pipeline that builds it on Windows, macOS, and Linux.

> All artifacts are unsigned development builds. The macOS Audio Unit is
> ad-hoc signed and validated in CI with `auval -v aufx Dsre BoCo`.

## Highlights

- 🎛️ **9-stage character engine** — Input trim → Body (low-shelf tone) →
  Heat/Desire (blended tanh/soft-knee waveshaper) → Motion (real stereo
  movement via phase-offset delay lines) → Silk (harshness-taming lowpass) →
  Width (mid/side, with a safety soft-clip) → equal-treatment dry/wet → Output
  trim.
- 🌐 **WebView-hosted UI, not a mockup** — the HTML/CSS/JS in `Prototype/` is
  the shipped interface, embedded in the plugin binary with no runtime
  filesystem or network dependency, and it also runs standalone in a browser
  with mocked data for fast iteration.
- 📈 **A response curve that's actually live** — reflects Body/Silk in real
  time and has two draggable handles wired to the same parameters as the
  knobs below it.
- 💾 **Presets, A/B, undo/redo** — 4 factory presets across all 3 modes, user
  presets saved to disk, two independent A/B slots with copy, full undo/redo.
- 🛡️ **Real-time safety, proven, not assumed** — a dedicated test overrides
  `operator new` to prove the audio thread performs zero heap allocations
  across a varied parameter sweep.
- 🧪 **22 automated tests** — 10 C++ DSP invariant tests, 2 preset-manager
  integration tests (through the real `APVTS → DesireEngine` path), 12 JS
  tests for every pure UI math function (meters, curve, knob mapping).
- 🩹 **A fallback that isn't an afterthought** — `NATIVE`/`AUTO` UI backend
  modes mean a missing WebView2 runtime degrades to a diagnostic control
  panel instead of a blank or crashed editor.

## What's in the plugin

### DSP (`Source/DesireEngine.cpp`)

| Parameter | Range | Default | What it does |
|---|---|---|---|
| Input | −24…+12 dB | **−10 dB** | Trim into the drive stage — the main gain-staging control. |
| Body | 0–100% | 65% | Low-shelf weight/density around 220 Hz, mode-biased. |
| Heat | 0–100% | 72% | Waveshaper hardness (blends a soft tanh knee with a harder knee). |
| Silk | 0–100% | 58% | Lowpass (5.5–20 kHz) that tames high-frequency harshness from saturation. |
| Desire | 0–100% | 76% | Main drive amount into the waveshaper — up to 12 dB of pre-gain. |
| Motion | 0–100% | 64% | Depth of two phase-offset (90°) fractional delay lines — real stereo decorrelation, not amplitude modulation. |
| Width | 0–200% | 128% | Mid/side stereo width on the wet signal, safety-clipped so anti-phase content at extreme width can't blow past a sane ceiling. |
| Mix | 0–100% | 42% | Dry/wet blend. |
| Output | −24…+12 dB | 0 dB | Final trim, applied after the blend. |
| Mode | Intimate / Club / After Dark | Club | Biases Body/Heat tone-shaping per character. |
| Bypass | off/on | off | True unity passthrough (crossfades Output back to 0 dB too, not just the wet mix) — no click. |

Every parameter has its own smoothing ramp (20 ms for Input/Output/Mix up to
225 ms for Motion, 200 ms for Mode changes) instead of one shared constant, so
a preset switch reads as a transition rather than a reset. All parameter
inputs are sanitized against NaN/Inf; `AudioProcessor::reset()` clears filter
and delay-line state on transport stop/loop.

### Metering & analysis

- Peak + RMS meters for input and output, **dB-scaled** (not linear — a
  linear meter drew a signal at −6 dBFS, already close to clipping, at only
  half height). The input meter reflects the signal *after* the Input trim is
  applied, so turning that knob visibly moves it.
- Spectrum analyzer: lock-free SPSC FIFO → 2048-point Hann-windowed FFT on the
  message thread → 96 log-spaced bins, selectable PRE/POST tap, and it's
  fully inert (no FFT work at all) when switched off.
- Low/mid/high band energy, gain-reduction, and a composite "desire energy"
  value are sent to the UI for visual feedback — none of it feeds back into
  the DSP.

### Interface

- 9 continuous knobs + Mode selector + Bypass, all bound through JUCE's web
  relay/attachment system — parameter changes are bidirectional with host
  automation, with one source of truth (the APVTS), never a second copy in
  JavaScript.
- Double-click any knob (or curve handle) to restore its true default value.
- Toolbar: preset prev/next/name/save, A/B + copy, undo/redo, bypass — built
  as transparent hit-targets over the baked SVG chrome, so nothing is drawn
  twice.
- Analyzer controls: Spectrum on/off, Pre/Post tap, LF/HF range.
- A JS error overlay + `reportJavaScriptError` native function surface UI
  bugs instead of failing silently.

### Presets, A/B, undo/redo

- 4 factory presets — **Late Night Confessions** (default), Velvet Room, Main
  Floor, After Hours — spanning all three modes.
- User presets save/load/delete as XML files on disk.
- Two independent A/B slots holding the full parameter state, with a
  one-click copy between them.
- Full undo/redo via `juce::UndoManager`, wired into the APVTS.

### Backend resilience

- `BORATO_DESIRE_UI_BACKEND` build option: `WEBVIEW` (default), `NATIVE`
  (diagnostic-only `juce::Graphics` editor, no WebView2 dependency), or
  `AUTO` (starts WebView2, falls back to the diagnostic editor automatically
  if the page hasn't reported ready within 3 seconds).
- The DSP never depends on the editor: audio keeps processing with the editor
  closed, and closing/reopening it can't affect sound.

## Fixed during development

- Toolbar controls visually doubling up on top of the baked SVG chrome.
- A hidden heap allocation on the audio thread every block (filter
  coefficient updates) — now allocation-free, proven by test.
- Unbounded gain through the stereo width stage on anti-phase content at
  extreme Width settings.
- Bypass leaking the Output trim instead of being a true unity passthrough.
- An audible click on Mode switches from an unsmoothed tone-shaping bias.
- The default preset pushing already-mastered/hot material into constant
  ceiling saturation (halved the internal drive ceiling, dropped default
  Input to −10 dB).
- VU meters reading on a linear scale instead of dB, and the input meter not
  responding to the Input knob at all (it measured the signal before the
  knob's gain was applied).
- Motion using an inaudible mono amplitude-modulation trick instead of actual
  stereo movement.
- The response curve being fully decorative with two non-functional handles.

## Downloads

| Platform | Files |
|---|---|
| Windows (x64) | `BoratoDesire-v0.0.1-Windows-x64-VST3.zip`, `-Standalone.zip` |
| macOS (Apple Silicon) | `BoratoDesire-v0.0.1-macOS-arm64-VST3.zip`, `-AU.zip` |
| macOS (Intel) | `BoratoDesire-v0.0.1-macOS-intel-VST3.zip`, `-AU.zip` |
| Linux (Ubuntu) | `BoratoDesire-v0.0.1-Ubuntu-VST3.tar.gz` |
| All | `INSTALL-*.md` (per-platform install paths) |

See [`README.md`](README.md) for building from source and
[`MANUAL.md`](MANUAL.md) for how to use the plugin.

## Testing

```
cmake --build build --config Release --target DesireEngineTests PresetManagerTests
ctest --test-dir build -C Release --output-on-failure
node --test tests/graphics-mapping.test.js
```

22 tests, all green: 10 DSP engine invariants, 2 preset-manager integration
tests, 12 UI mapping-function tests.

## Changelog

- `feat: add JUCE WebView plugin with DSP engine, presets, and tests`
- `feat: add multi-platform CI release build workflow`
- `feat: add README, user manual, and license documentation`

Initial release — no prior tag to diff against.
