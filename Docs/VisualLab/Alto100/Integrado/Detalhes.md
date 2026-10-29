# Experimentos integrados — protocolo

1920×1080, ScreenPercentage=100, resolução dinâmica desligada, GTX 1650, DX12, Development -game. Câmera fixa (0,-312.028006,250.352636), pitch -15, yaw 90, FOV 90, igual ao bloco de sombras e GI/AO. Base provisória: Alto, TSR Q1/Flickering=0, Lumen Irradiance Field Gather, sombras 3×1536, alcance efetivo 42,5 m, cortes pequenos aprovados provisoriamente. Sem alterar vento, animações ou layout salvo.

Cada candidato: 30 s de aquecimento, três capturas reais de 30 s, 5 s de intervalo. Prints e ProfileGPU fora das janelas de medição. FPS = inverso da média do frametime; 1% low = inverso da média dos 1% frames mais lentos; tempos GPU = medianas dos CSVs. Custos individuais de ProfileGPU são snapshots, não medianas.

## AO

SSAO separado não executa com Irradiance Volume nesta configuração. O Lumen usa ShortRangeAO, já em meia resolução (DownsampleFactor=2): 2 slices e 3 steps. Variante leve: 1 slice, 2 steps. Variante desligada serve como limite máximo de economia, não como recomendação visual. ProfileGPU: passe ShortRangeAO 0,240 ms atual e 0,129 ms leve. Não confundir com os ~2,09 ms de SSAO no experimento anterior sem Lumen.

AO assado em rochas/troncos/paredões não foi criado: o A/B do passe atual demonstra a pequena margem disponível. Nenhuma textura/vertex color original foi alterada. A grama continua sem AO extra no material.

## Arbustos

Cadeias de materiais copiadas integralmente para /Game/HypeWorld/VisualLab/Integrado/Arbustos. BaseColor original multiplicado por gradiente local de altura (0,62 a 1) e escurecimento suave do centro (até 22%, enfraquecendo no topo). Isso é AO falso artístico no material, não bake de iluminação. Materiais de flores preservados.

Teste separado de contact shadow: 12 cm em world space, intensidade 0,5, apenas arbustos como casters; grama excluída. Flags e parâmetros originais restaurados entre variantes. A mudança é runtime, não salva na luz ou no mapa. Comparar sua qualidade e custo, sem assumir que contact shadow funciona somente pelo valor configurado.

## Copa

Uma árvore CommonTree_3 visível no enquadramento, índice e transform em selecionadas.json. Malha copiada: tronco original preservado; 1800 triângulos da seção de folhas removidos e substituídos por sete volumes arredondados opacos. Material Default Lit, sem alpha, degradê vertical e variação procedural suave de cor. É um protótipo de massa de copa, não arte final pintada à mão.

Um componente de foliage vazio do mapa copiado serve de suporte temporário. A instância original fica com escala zero durante o A/B e retorna ao transform original. O layout e o mapa não são salvos. A medição de uma árvore não pode ser extrapolada linearmente para todas as árvores.

O componente vazio precisa de mobilidade Movable para aceitar a troca em runtime. O resultado mede o protótipo completo, incluindo essa diferença de componente; não é uma medida puramente isolada do custo de alpha. A copa foi conferida renderizada antes da série final. Tentativas anteriores com material de fallback ou troca de mesh recusada foram interrompidas e não entram no resultado. Materiais usam VertexInterpolator para levar a posição local ao pixel shader.

## FSR

Plugin oficial AMD 4.1.1, pacote UE 5.8, em Saved/VisualLab/Tools/AMD/UE58/FSR. BuildId compatível 55116800. Carregado por linha de comando, sem habilitação no .uproject ou instalação global. Native AA QualityMode=0; ScreenPercentage=100; Frame Generation=0; swapchain override=0; sharpening=0. Fonte: https://gpuopen.com/learn/ue-fsr/ .

Vídeos terão passos de simulação fixos de 1/30 s, separados da medição real de desempenho. Input de movimento usa a lógica de corrida existente, sem editar animações. O vento segue os parâmetros originais. A câmera permanece fixa para ambas as opções.

Atualização: o usuário cancelou a etapa de vídeos. A gravação não foi concluída; não há validação de ghosting ou cintilação em movimento nesta entrega. Apenas prints e benchmarks entram no resultado. A orientação do personagem variou em alguns prints; a câmera e a resolução permaneceram fixas.

## Retomada

As três medições de AO e os candidatos controle/arbustos/material+contact terminaram antes da interrupção. A referência a um componente transitório do personagem expirou antes do teste de copa; as capturas de copa não chegaram a ocorrer nessa série. As flags de contato passam a ignorar referências destruídas.

A série final possui novo controle TSR Q1, copa opaca, FSR Native AA e combinação candidata, todos medidos na mesma sessão e câmera. A combinação inclui Irradiance Volume, sombras provisórias, AO original e arbustos com material/contact, usando FSR Native AA. A copa permanece fora da combinação pelo resultado visual insatisfatório. A combinação é experimental e não equivale à aprovação do usuário. Não somar deltas isolados para estimar esse resultado.
