# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

**Borato Desire** — a JUCE audio plugin (saturation/character effect) currently in the **design/prototyping phase**. The repo contains the UI design system, an interactive HTML/JS prototype, and empty C++ stubs for the eventual JUCE implementation. There is **no build system yet** (no CMakeLists, no .jucer, no headers) — do not invent build/test commands; all files in `Source/UI/` are empty placeholders waiting for implementation.

Docs and comments are in a mix of Portuguese and English.

## Running the prototype

The prototype is plain HTML/JS/CSS with no dependencies or build step. It references assets via `../Design/...`, so serve from the **repo root**:

```
python -m http.server 8000
# then open http://localhost:8000/Prototype/
```

(Opening `Prototype/index.html` directly via `file://` also works.)

## Architecture: the design → prototype → JUCE pipeline

The core idea of this repo is a three-stage pipeline. Every visual element flows through it, and the naming conventions tie the stages together:

1. **`Design/`** — source of truth.
   - `tokens/*.json` — colors, dimensions, typography, animation values. These are mirrored as CSS variables in `Prototype/styles/tokens.css` and are intended to become `DesireColours::` constants in C++. Change tokens here first, then propagate.
   - `static-svg/` — baked, non-animated layers (shell, knob bases, frames). In the prototype these are `<img>` layers; in JUCE they'll be drawn once/cached.
   - `juce-paint-specs/` — **the JUCE implementation contract.** Each dynamic element has a paired `.md` + `.svg`. The `.md` gives exact geometry (bounds, radii, angles, stroke widths), colors by token name, effects, and names the target C++ method (e.g. `DesireKnob::paintValueArc()`, `DesireDisplay::paintSpectrum()`). When implementing C++ paint code, these specs are the reference — not the prototype code.
   - `textures/` — webp textures (glass, metal, club backdrop) used as overlay layers.

2. **`Prototype/`** — living reference for *dynamic behavior* (the specs cover static geometry; the prototype defines motion and interaction). Layered structure in `index.html`: static shell SVG → display (spectrum → dancer image → curve → particles, inside a 1156×540 SVG) → glass overlay → control layer (knobs/meters absolutely positioned).

3. **`Source/UI/`** — one `.cpp` per component, names matching the spec targets: `DesireEditor`, `DesireLookAndFeel`, `DesireKnob`, `DesireDisplay`, `DesireCurve`, `DesireSpectrum`, `DesireDancer`, `DesireStereoMeter`. All currently empty.

`Assets/` holds the original dancer artwork iterations (the traced `-juce-paint` version is the one meant for implementation).

## Canonical geometry and behavior (from specs + prototype)

- Plugin: 1456×1024. Display: 1156×540. Main knob viewBox 200×200 (arc r=86); small knobs 110×110 (arc r=48).
- Knobs rotate −135° to +135°, normalized 0–1. Value arc drawn via partial arc (prototype uses `stroke-dasharray` over the 270° arc); each arc is a thin white core stroke plus a wider colored glow stroke.
- Knob interaction: vertical drag, 250px for full range.
- Parameters: `input`, `body`, `heat`, `silk`, `desire` (main), `motion`, `width`, `mix`, `output`. Readout formats: input/output map 0–1 → −24..+12 dB; width shows 0–200%; the rest show 0–100%. Accent colors: pink (most), cyan (motion/width), magenta (mix).
- Meters: LED gradient green → yellow → pink → red, revealed bottom-up by clipping to the audio level.
- The dancer requires a bottom fade-out mask (last 20%) — see `Design/juce-paint-specs/borato-desire-dancer-juce-paint.md` for the full layer stack and the 4s "breathing glow" animation cycle.
