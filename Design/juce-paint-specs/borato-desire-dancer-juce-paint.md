# Desire Pole Dancer Neon

- Asset Original: `borato-desire-dancer-polished.svg`
- Bounding Box no JUCE: 333x520 (Escala ~0.585 do SVG original 570x922)
- Posição (relativa ao Display de 1156px): Deslocada sutilmente para a esquerda, `X = 450px`. O mastro (pole) fica fiel ao conceito inicial do `first-drawing.svg`.
- Z-Index: Entre o espectro (fundo) e as partículas/curva (frente).
- **Fade Mask**: É OBRIGATÓRIO aplicar uma máscara de opacidade (`juce::Image` com gradient Alpha) na base do rendering da dançarina para que ela suma gradualmente nos últimos 20% inferiores. Isso previne o corte abrupto nas pernas e mastro, integrando-a com textos ou gráficos subjacentes.

## Layers para o `juce::Graphics` (Top to Bottom — ids iguais aos do SVG):
1. **Hair Strands** (`hair-strands`): paths individuais em ondas S, strokes `#d93387` / `#ff4aa4` / `#ff54b4`, widths 1.6–2.8, opacity 0.85. Glow do cabelo em `hair-glow-juce-paint` (mesmos 4 primeiros paths, stroke 4.2, `#ff2b91`, opacity 0.24, blur 2.4px).
2. **Core Highlights** (`contour-core-highlight-juce-paint`): strokes finos `#ffe2f0` 0.9px sobre perfil do rosto, seio, glúteo, joelho e pé — o "filamento quente" do neon.
3. **Inner Contours** (`contour-lines`): linhas anatômicas (queixo/mandíbula, olho, clavícula, seio, umbigo, vincos), stroke `#ff66b1`, width 2, opacity 0.8. Glow correspondente em `contour-glow-juce-paint` (blur 2.4px, opacity 0.3).
4. **Outer Rim** (`outer-rim` + `outer-rim-near-glow-juce-paint` + `outer-rim-wide-glow-juce-paint`): o MESMO path aberto desenhado 3x — core 2.2px, near glow 3.6px (blur 3.3px, opacity 0.85), wide glow 7px (blur 10px, opacity 0.55). Gradiente `#ff8cc8 → #ff4ca6 → #ff2b91 → #ff70b7`. O path pula (via `M`) o trecho crânio/nuca coberto pelo cabelo.
5. **Body Fill** (`body-fill`): path FECHADO da silhueta, `juce::ColourGradient` látex/vidro escuro de `#210817` para `#07050c`.
6. **The Pole** (`pole`, ATRÁS da dançarina): rect 13px preenchido com `juce::ColourGradient` horizontal cromado (`#191c23 → #8f99ab → #e8eef6 → #6a7383 → #14161c`) + linha core `#ffd9ec` 1.2px e reflexo rosa `#ff4aa2` 1.8px opacity 0.45.

## Animação Dinâmica (Breathing Glow)
- A intensidade das camadas de brilho oscila ("respira").
- **Ciclo:** 4 segundos (`ease-in-out infinite alternate`).
- **Valores:** Simulado via CSS `filter: brightness(0.9) -> 1.2` e variação do raio do drop-shadow. No JUCE, animar a opacidade das camadas "Near Glow" e "Wide Glow" por um `juce::Timer` atrelado ao ciclo de 4s.

## Histórico de Correção (V4)
- **Método**: Extração matemática 100% automatizada (scikit-image + Schneider's Bezier Fitting) a partir do mockup raster `mock-first-ideia.png`.
- **Fidelidade Matemática**: Média de desvio (Chamfer distance) de **1.753 px** em relação aos traços originais do artista.
- **Geometria Uniforme**: Aspect ratio original rigorosamente mantido (sem distorção vertical ou horizontal), com o mastro (pole) alinhado em `X = 190` e base em `Y ≈ 900` no viewBox `570x922`.
