# Borato Desire — User Manual

Borato Desire is a character/saturation plugin. It's built to add body,
warmth, harmonic texture, stereo movement, and a sense of intimacy or
proximity to a track — not to work as an EQ, exciter, compressor, or generic
distortion.

## Installing

**VST3** — copy `Borato Desire.vst3` to:

```
C:\Program Files\Common Files\VST3\
```

Then rescan plugins in your DAW.

**Standalone** — run `Borato Desire.exe` directly, no installation needed.
Pick your audio interface from the Options menu in the top-left corner.

If the plugin opens to a plain diagnostic screen with just a handful of
sliders instead of the full interface, the WebView2 runtime couldn't load
(rare on an up-to-date Windows 10/11 machine). Audio still passes through
correctly in that state — see "WebView2 runtime" below to get the full UI
back.

## The toolbar

| Control | What it does |
|---|---|
| **BORATO COMPANY** logo | Decorative, top-left. |
| **‹ Preset name ›** | Step through presets, or click the name area. |
| **SAVE** | Save the current settings as a new preset. |
| **A / B** | Two independent slots for the full parameter state. Click to switch which one is live. |
| **COPY A↔B** | Copy the currently active slot's settings into the other one — a quick way to start a variation from where you are. |
| **BYPASS** | Bypasses the effect with a short crossfade (no click), including the Output trim — a bypassed plugin is a true unity passthrough. |
| **↶ / ↷ (undo/redo)** | Undoes/redoes parameter changes. |

## The display

The dancer graphic is decorative. The **pink curve** running through it is
not: it's a live plot of what your **Body** and **Silk** settings are doing to
the tone, and it redraws in real time as you turn those knobs. It also has two
draggable handles — one for Body, one for Silk — so you can shape the tone
directly on the curve instead of the knobs below; both control paths hit the
exact same parameter, so they always agree. Double-click a handle to reset
that parameter to its default, same as double-clicking a knob.

Below the curve, the **SPECTRUM / PRE / POST / LF / HF** panel controls the
frequency analyzer overlay: turn it off if you don't need it (it costs
nothing when off), choose whether it taps the signal before or after
processing, and narrow the visible frequency range with the LF/HF sliders.

**INTIMATE / CLUB / AFTER DARK** along the bottom select the processing Mode
(see below) — click, or use arrow keys once one has focus.

## Knobs

All knobs: drag vertically to change (up increases), double-click to reset to
its default value. The main **DESIRE** knob in the center is intentionally
the biggest control — it's the one meant to be reached for first.

| Knob | Range | What it does |
|---|---|---|
| **Input** | −24…+12 dB | Trim before the drive stage. This is what determines how hard the saturation is driven — turn it down on already-loud/mastered material, up on quiet sources. Defaults to −10 dB precisely so a hot master doesn't immediately slam into heavy saturation. |
| **Body** | 0–100% | Weight and density in the low-mids, without turning to mud. Also one of the two knobs shown live on the response curve. |
| **Heat** | 0–100% | Saturation character/hardness — how aggressive the harmonic drive is. |
| **Silk** | 0–100% | Smooths harsh high-frequency content that saturation tends to introduce. Higher Silk = more smoothing, not more "brightness." The other knob shown on the response curve. |
| **DESIRE** (main) | 0–100% | The primary drive amount into the saturation stage. This is the one knob to reach for to make the effect more or less present overall. |
| **Motion** | 0–100% | Slow, subtle stereo movement — two delay lines drifting apart in time. At 0% it's transparent; higher settings should read as gentle motion, not chorus or vibrato. |
| **Width** | 0–200% | Stereo image width via mid/side processing. 100% = the original image untouched, 0% = mono, up to 200% = exaggerated. Works on the processed (wet) signal, so its audible effect scales with Mix. |
| **Mix** | 0–100% | Dry/wet blend. |
| **Output** | −24…+12 dB | Final trim, applied after the dry/wet blend. |

## Modes

**Intimate** — proximity and warmth, less width, less movement, a softer
saturation knee. **Club** — the default, most neutral/balanced character.
**After Dark** — denser, darker, a harder saturation knee, deeper and slower
Motion. Switching modes crossfades over ~200 ms rather than jumping.

## Bypass and gain staging

Bypass is a true unity passthrough — it doesn't just mute the wet signal, it
also brings Output back to 0 dB, so a non-default Output trim can't still
color a "bypassed" signal. If a preset or setting is clipping/distorting on
material that was already close to 0 dBFS, start by pulling **Input** down —
that's the control that determines how hard the drive stage is hit, and it's
usually the fastest fix.

## WebView2 runtime

The plugin's interface is hosted in Microsoft WebView2, already installed on
essentially all current Windows 10/11 systems. If the plugin shows the plain
diagnostic screen instead of the full UI, install the
[WebView2 Evergreen Runtime](https://developer.microsoft.com/microsoft-edge/webview2/)
and reopen the plugin.
