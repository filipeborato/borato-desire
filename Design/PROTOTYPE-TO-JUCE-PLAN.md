# Borato Desire — plano do protótipo ao plugin JUCE

## 1. Entendimento do problema

O objetivo visual é aproximar o protótipo de Design/reference/mock-first-ideia.png sem transformar ilustrações estáticas em componentes JUCE desnecessários.

Estado encontrado em 23/07/2026:

- o repositório ainda está na fase de design/protótipo;
- Source/UI contém somente placeholders vazios e ainda não existe CMakeLists.txt;
- os fundos WebP estavam referenciados pelo caminho errado no CSS e o backdrop ficava atrás do fundo opaco;
- a dançarina canônica é Assets/borato-desire-dancer-art.svg;
- o trace simplificado Design/static-svg/borato-desire-dancer-polished.svg não faz parte do fluxo;
- o SVG canônico inclui um painel escuro em todo o viewBox, origem da moldura retangular;
- o espectro anterior usava 96 barras grossas e movimento sintético excessivo;
- partículas eram criadas e destruídas continuamente em animações independentes.

## 2. Arquitetura proposta

### Camadas estáticas

Permanecem SVG/PNG/WebP e entram no plugin por juce_add_binary_data:

- shell e toolbar;
- fundo da boate, reflexo e ruído de vidro;
- display frame e textos decorativos, sem a barra de modos e sem o painel do analisador;
- bases dos knobs;
- dançarina canônica, por meio de export transparente derivado 1:1 da arte original.

Essas imagens não precisam virar um Component cada uma. O DesireEditor e o DesireDisplay devem carregar os Drawables/Images uma vez no construtor e reutilizá-los em paint().

### Componentes JUCE

Somente elementos interativos ou ligados a dados em tempo real:

- DesireEditor: composição, escala e ciclo de vida;
- DesireDisplay: fundo cacheado e composição do campo central;
- DesireSpectrum: visualização dos bins fornecidos pela ponte de análise;
- DesireAnalyzerControls: view, PRE/POST e faixa LF/HF como estado persistente de UI;
- DesireCurve: resposta agregada dos parâmetros/DSP;
- DesireModeSelector: três segmentos interativos ligados a um AudioParameterChoice;
- DesireKnob: controle, arco, ponteiro, leitura e attachment APVTS;
- DesireStereoMeter: níveis L/R;
- controles de preset, A/B, undo/redo e modos.

### Fluxo de dados

Audio thread:

- processa DSP sem UI, imagens, locks, I/O ou alocações;
- publica meters por atomics;
- copia amostras para FIFO SPSC pré-alocada quando a análise estiver habilitada.

Message thread:

- consome a FIFO e calcula/normaliza o espectro;
- lê os atomics dos meters;
- atualiza no máximo a 30 Hz;
- recalcula a curva somente quando parâmetros relevantes mudarem;
- desenha assets já decodificados e caches já construídos.

### Layout

Adotar o mesmo padrão dos projetos Borato EQ e Borato Gravitas:

- canvas de design fixo em 1456 × 1024;
- aspect ratio travado;
- função mapRect para converter coordenadas do mock em bounds reais;
- componentes filhos posicionados em resized();
- LookAndFeel central para tokens, tipografia e estados.

## 3. Riscos identificados

- Real-time: calcular FFT, carregar SVG ou reconstruir Paths no audio thread causaria glitches.
- Visual: aplicar mix-blend-mode ao display inteiro mistura também curva e espectro; o blend deve ficar restrito à arte que precisa dele.
- Asset: JUCE não deve depender do filtro de dark-key usado pelo navegador. Antes da implementação C++, exportar uma versão transparente fiel à arte canônica, sem retrace artístico.
- Performance: filtros borrados grandes, DOM churn e repaint total a 60 Hz são desnecessários.
- Manutenção: arquivos vazios como Design/textures/smoke.webp, Design/static-svg/borato-desire-control-panel.svg e Design/master/borato-desire-master-preview.svg não devem entrar no BinaryData.

## 4. Implementação em fases

### Fase A — protótipo visual

1. Corrigir caminhos e ordem explícita das camadas.
2. Usar Assets/borato-desire-dancer-art.svg.
3. Remover visualmente o painel escuro com dark-key e fades no protótipo.
4. Reduzir o espectro a barras finas, determinísticas e discretas.
5. Usar curva Bézier estável próxima ao mock.
6. Criar partículas uma vez, sem alocação por frame.
7. Ajustar tamanho e posição de fundo, dançarina, curva e painel após comparação renderizada.

### Fase B — higiene de assets

1. Preservar a arte canônica original.
2. Gerar export transparente derivado, sem simplificação ou redesenho.
3. Validar transparência, bounds e escala contra o mock.
4. Remover do fluxo arquivos vazios, backups e experiências descartadas.
5. Definir no CMake apenas os assets realmente usados.

### Fase C — esqueleto JUCE

1. Criar CMakeLists.txt e juce_add_plugin seguindo os projetos irmãos.
2. Criar BoratoDesireAssets com juce_add_binary_data.
3. Implementar PluginProcessor, APVTS e IDs de parâmetros.
4. Implementar DesireEditor, layout escalável e LookAndFeel.
5. Desenhar a primeira tela somente com assets estáticos para validar fidelidade.

### Fase D — componentes dinâmicos

1. Knobs e attachments.
2. Meters com atomics e ballistics.
3. Curva vinculada aos parâmetros, inicialmente somente de visualização.
4. Seletor Intimate/Club/After Dark ligado ao parâmetro de modo.
5. Painel interativo do analisador, sem expor configurações visuais como parâmetros automáveis.
6. FIFO/analisador e espectro real.
7. Presets, A/B e undo/redo.

### Fase E — DSP e validação

1. Implementar o grafo de saturação após fechar contratos dos parâmetros.
2. Aplicar smoothing em automações audíveis.
3. Testar silêncio, extremos, automação rápida e mudanças de sample rate.
4. Medir custo da UI e DSP separadamente.
5. Validar Standalone e formatos de plugin antes de empacotar.

## 5. Critérios de validação visual

- fundo da boate visível e escurecido como suporte, sem dominar a interface;
- nenhuma borda retangular ao redor da dançarina;
- dançarina correta, sem retrace simplificado;
- curva atrás da dançarina e próxima à geometria da referência;
- espectro com linhas finas, baixa opacidade e movimento sutil;
- partículas discretas;
- shell, display, knobs e meters alinhados ao canvas 1456 × 1024;
- nenhuma dependência de arquivo externo em runtime no plugin final.

## 6. Decisões de engenharia

- SVG/PNG estático continua sendo SVG/PNG no produto final.
- Componente JUCE é reservado a interação, estado ou visualização dinâmica.
- Assets são decodificados fora de paint() e nunca no audio thread.
- O protótipo é referência de comportamento; a referência visual e os assets aprovados são a fonte de fidelidade.
- A análise espectral final deve refletir áudio real. O sinal sintético do protótipo é apenas uma prévia visual contida.
