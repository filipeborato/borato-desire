# Desire Pole Dancer Neon

- Asset Original: `borato-desire-dancer-polished.svg`
- Bounding Box no JUCE: 333x520 (Escala ~0.585 do SVG original 570x922)
- Posição (relativa ao Display de 1156px): Deslocada sutilmente para a esquerda, `X = 450px`. O mastro (pole) fica fiel ao conceito inicial do `first-drawing.svg`.
- Z-Index: Entre o espectro (fundo) e as partículas/curva (frente).
- **Fade Mask**: É OBRIGATÓRIO aplicar uma máscara de opacidade (`juce::Image` com gradient Alpha) na base do rendering da dançarina para que ela suma gradualmente nos últimos 20% inferiores. Isso previne o corte abrupto nas pernas e mastro, integrando-a com textos ou gráficos subjacentes.

## Layers para o `juce::Graphics` (Top to Bottom):
1. **Hair Strokes**: Paths individuais, stroke `#d93387` ou `#ff4aa4`, opacity 0.82
2. **Inner Contours**: Paths de contorno do corpo, stroke `#ff54b4`, stroke-width 3
3. **Core Glow**: Path `dancer-glow-juce-paint` e `contour-glow-juce-paint`, com intenso filtro blur (`juce::ImageConvolutionKernel`).
4. **Near Glow & Wide Glow**: Path `outer-rim`, com strokes grossos `#ff4ca6` e fortes opacidades.
5. **The Pole**: Um rect 3D (largura 16px) preenchido com `juce::ColourGradient` horizontal simulando Reflexo Metálico/Cromo (tons de azul/cinza até rosa).
6. **Body Fill**: Path `body-fill`, `juce::ColourGradient` simulando látex/vidro escuro (de `#3d152c` para `#07050c`), possuindo ainda um leve contorno interno (`stroke="#ff70b7"`) para dar volume.

## Animação Dinâmica (Breathing Glow)
- A intensidade das camadas de brilho oscila ("respira").
- **Ciclo:** 4 segundos (`ease-in-out infinite alternate`).
- **Valores:** Simulado via CSS `filter: brightness(0.9) -> 1.2` e variação do raio do drop-shadow. No JUCE, animar a opacidade das camadas "Near Glow" e "Wide Glow" por um `juce::Timer` atrelado ao ciclo de 4s.
