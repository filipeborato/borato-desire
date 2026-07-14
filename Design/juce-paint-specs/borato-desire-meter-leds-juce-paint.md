# Desire Stereo Meter LEDs

- Bounds: 0, 0, 32, 466
- L Channel Rect: 2, 2, 12, 462
- R Channel Rect: 18, 2, 12, 462
- Gradient Base: `DesireColours::green` (#00FF7F)
- Gradient Middle: `DesireColours::yellow` (#FFD700)
- Gradient High: `DesireColours::pink` (#FF318F)
- Gradient Peak: `DesireColours::red` (#FF0000)
- JUCE update: In `paint()`, fill the rects entirely with the linear gradient, but use `g.reduceClipRegion()` from the bottom up based on the normalized audio level (0.0 to 1.0) so only the "lit" portion is drawn.
