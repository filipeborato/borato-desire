# Desire Analyzer Controls

- **Contrato de componente:** implementar como `DesireAnalyzerControls`.
- **Bounds:** x 946, y 24, width 178, height 138 no display 1156 × 540.
- **View:** SPECTRUM ou OFF; OFF suspende repaint/análise visual quando possível.
- **Tap:** PRE ou POST; escolhe a fonte de amostras exibida, sem alterar o áudio.
- **LF/HF:** limites logarítmicos de 20 Hz a 20 kHz.
- **Estado:** persistir na árvore de estado do editor/processador, mas não criar
  parâmetros automáveis da DAW enquanto esses valores forem somente visuais.
- **Thread:** interação e repaint na message thread. A audio thread publica
  somente amostras/níveis por estruturas lock-free pré-alocadas.
