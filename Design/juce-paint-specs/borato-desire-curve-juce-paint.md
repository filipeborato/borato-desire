# Desire Processing Curve

- Bounds: 0, 0, 1156, 540
- Render Method: `juce::Path` representing the aggregate frequency magnitude response (EQ + Saturação).
- Geometry: Smooth bezier/spline curve across the width.
- Stroke: `DesireColours::magenta`, thickness 4.0f.
- Effect: Outer glow (DropShadow or duplicated blurred path with 8px radius) to give it a neon tube look.
- Target: `DesireDisplay::paintCurve()`
