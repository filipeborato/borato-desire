# Desire Dancer / Particles

- Bounds: 0, 0, 1156, 540
- Render Method: Drawing multiple `juce::Path` circles (or using OpenGL points) representing atmospheric glow/dust.
- Behavior: Spawned randomly at the bottom, floating upwards with varying speeds. Size 2px to 8px.
- Color: `DesireColours::cyan` (representing Motion/Silk).
- Effect: `blur` or radial gradients for soft edges. Opacity decays over life.
- Target: `DesireDisplay::paintParticles()`
