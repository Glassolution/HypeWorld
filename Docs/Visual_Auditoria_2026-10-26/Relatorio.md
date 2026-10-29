# Bloco 0 — auditoria visual, 26/10/2026

Estado: diagnóstico para revisão; nenhum visual novo aprovado. Nenhum material, mapa, animação, vento ou configuração foi alterado. Sem commit de aprovação. As quatro fontes foram copiadas para esta pasta. `medir.py` reproduz as medições e a prancha; `medicoes.json` contém os recortes exatos.

## Evidências e limites

Base: `Docs/Visual_Aprovado.md`, histórico até `e3f369d`, working tree, configurações do projeto e `UE_5.8/Engine/Config/BaseScalability.ini`, scripts de integração do Indra e imagens fornecidas. Há alterações de animação/combate preexistentes no working tree: não foram tocadas.

Os assets de grama/terreno não têm diferenças entre a aprovação `50a4980` e HEAD, nem alterações locais detectadas pelo git. O mapa `L_Tutorial.umap` mudou em `e3f369d`. A inspeção read-only com UnrealEditor-Cmd, NullRHI e Python foi concluída: `assets_atuais.json`, `parametros_efetivos.json` e `comparacao_mapas.json`. Uma cópia exata do objeto LFS do mapa em `50a4980` foi colocada em `Content/HypeWorld/VisualLab/Auditoria/L_Aprovado_50a4980.umap` e comparada sem salvar nenhum mapa. O PostProcessVolume completo e as propriedades coletadas de sol, SkyLight e fog coincidem; SkyAtmosphere foi identificado, mas suas propriedades específicas não foram expostas por esta coleta e continuam pendentes. Não houve renderização ou medição de GPU; NullRHI não serve para benchmark. Configurações efetivas durante jogo ainda precisam de captura runtime.

## O que regrediu, o que já era assim e por quê

| Item | Evidência | Conclusão |
|---|---|---|
| Tufos/espaços | Prancha anterior `visual_zenkai/grama_tufos/1_grama_angulo_img1.png` mostra espaços amplos no painel aprovado; imagem 1 atual mostra maior continuidade e detalhe fino | Diferença visual real, mas câmeras diferentes. Não há rollback comprovado dos assets. `Saved/Config/WindowsEditor/GameUserSettings.ini` está com foliage=3 (100%), contra 40% em Médio; hipótese forte para aumento de densidade, ainda não prova o preset do print |
| Altura/largura | Aprovação já define Z 0,36–0,55 e XY 1,1–1,4; +20% de altura já havia sido recusado | Grama baixa não é regressão recente comprovada. Sua nova solicitação muda esse alvo e autoriza preparar variantes em cópia |
| Cor da grama | Aprovação: ponta (0,28;0,55;0,12), base RVT, chão com brilho 2; materiais sem alteração desde `50a4980` | Aspecto mais amarelo/pálido que Zenkai confirmado nas amostras; não há evidência de nova LUT causando isso |
| Cor de arbustos | `16d3ef8` descreve degradê amarelado/verde-azulado; imagem 1 mostra arbustos claros e pouco saturados | Escolha anterior de paleta, não regressão posterior provada |
| Sombras | PCSS=1 desde `16d3ef8`; CSM 4×2048 já comentado em `a48e7a4`; Médio da engine usa 1×1024 | Limitação antiga aceita por custo. Não houve remoção recente demonstrada de PCSS |
| AA | `a48e7a4` mudou AA desligado (0) para TSR (4); continua 4 | TSR não foi desligado no arquivo. Médio usa history 100%, UpdateQuality 1 e Flickering=0; aliasing temporal permanece possível |
| Resolução | `a48e7a4` mudou padrão 100% para 67%, modo fixo; continua igual | Redução real histórica, anterior à última aprovação. Só ~45% dos pixels nativos; aumenta vulnerabilidade de lâminas finas |
| Pós-processo | Volume comparado pela engine com cópia do mapa em `50a4980`: propriedades iguais | Sem regressão detectada nesse volume. Exposição 1,4, saturação 0,9, Slope 0,82, bloom 0,5. Vinheta 0,6, mais forte que o novo pedido de vinheta leve. LUT=None e override desligado |
| Profundidade | `16d3ef8` criou clareira circular de 120 m com paredões | Oclusão pelo paredão é composição/geometria, não algo que fog sozinho resolva. Procurar um ângulo existente; não mover mapa |
| Indra | `e3f369d` introduziu Indra; script cria Default Lit com atlas, roughness 0,7 e specular 0,3; script de troca habilita stencil 1 no Indra | Novo material interage com PP já existente. Túnica muito clara confirmada; causa de perda de volume ainda requer isolamento material/PP/exposição |

O padrão `DefaultGameUserSettings.ini` só fixa GI e AA em Médio. Isso NÃO configura todos os grupos como Médio. O comentário cita Lumen, mas `r.DynamicGlobalIlluminationMethod=0`: comentário desatualizado, não evidência de GI ativa.

Leitura efetiva dos materiais: RVT Base Color ligado; normal map desligado; PDO e dither zerados; TufoEscala=120, TufoLimiar=0,36, TufoContraste=5, AreaEscala=420, AreaMin=0,6. Master Default Lit, duas faces. Cor da ponta e chão coincidem com o documento. **T_Grass_Opacity tem Scale Mips for Alpha Coverage=False**, LOD bias=0, mip pela texture group. É um candidato de teste confirmado, não prova isolada de cintilação. A textura pertence ao pack: qualquer alteração será em cópia.

Divergência documental: culling por variedade começa em 2700–2800 cm, termina em 4000 cm nos três primeiros clumps, 3200 no quarto e 3500 no quinto; o documento resume como 3000–4000. Como esses assets não mudaram desde a aprovação, isso é imprecisão do registro, não regressão nova.

Sol atual: intensidade 4, Source Angle 2,5°, cor RGB 255/250/240. Fog: density 0,012, height falloff 0,2, start distance 2000 cm. São os mesmos valores coletados na cópia aprovada.

67% significa aproximadamente 1286×724 internos para saída 1920×1080, ou 858×482 para 1280×720, se esse valor efetivamente estiver ativo. `sg.ResolutionQuality=0` salvo significa usar o padrão; não significa renderizar a zero. A resolução da imagem anexada não revela a resolução interna.

## Medições das referências

Medições em pixels sRGB das capturas, sem calibração de monitor, exposição ou compressão. Medianas por região; não são cores lineares para colar diretamente no material. H em graus, S/V em %. Luminância Y calculada após linearizar sRGB. Os retângulos estão no JSON. Regiões de cenas distintas não isolam uma única variável de iluminação.

| Região | RGB mediano | H | S | V | Y linear % |
|---|---|---:|---:|---:|---:|
| Atual, grama ao sol | 126,154,67 | 79,3 | 56,4 | 60,4 | 27,8 |
| Zenkai 2, grama ao sol | 99,151,29 | 85,3 | 81,0 | 59,2 | 24,9 |
| Atual, arbusto claro | 126,173,104 | 101,5 | 40,1 | 67,8 | 35,3 |
| Zenkai 2, sombra no campo | 26,53,19 | 108,0 | 63,5 | 20,8 | 2,8 |
| Zenkai 2, montanhas | 185,203,219 | 206,8 | 15,5 | 85,9 | 57,9 |
| Atual, túnica | 231,227,224 | 25,7 | 3,0 | 90,6 | 77,3 |
| Zenkai 3, braço | 180,186,190 | 192,9 | 5,2 | 74,3 | 48,9 |
| Zenkai 3, roupa | 27,26,32 | 235,0 | 18,8 | 12,9 | 1,1 |
| Zenkai 3, montanhas | 119,140,147 | 195,6 | 18,7 | 57,6 | 24,7 |
| Zenkai 4, rosto | 168,146,136 | 18,2 | 19,5 | 65,9 | 30,7 |
| Zenkai 4, roupa azul | 1,19,63 | 223,0 | 98,5 | 24,7 | 0,8 |
| Zenkai 4, calça | 167,163,169 | 270,0 | 4,3 | 66,7 | 37,4 |

Correções à análise inicial:

- A referência 2 tem grama mais saturada e um pouco mais verde, com brilho HSV semelhante. Não basta reduzir brilho ou saturação global. As sombras são bastante profundas, embora verdes; “macio” não significa baixo contraste em toda a cena.
- Os tufos da referência são mais legíveis e há pausas de chão. Não se pode determinar número de polígonos, uso de RVT, shader ou largura física a partir de screenshots. Esses são meios propostos para reproduzir a aparência, não fatos sobre Zenkai.
- O arbusto amostrado é verde (H≈102°), claro e dessaturado, não ciano. A correção deve controlar valor e saturação além de matiz.
- Imagem 2 mostra forte separação atmosférica, mas parte do branco das montanhas pode ser neve/material. Na imagem 3 as montanhas são menos claras; não aplicar a mesma intensidade de névoa a todas as cenas.
- Pele tem pouco detalhe de sombreado local, mas temperatura e luminosidade variam muito entre 3 e 4. Roupa azul 4 é muito escura/saturada, e dobras têm contraste perceptível. Evitar uniformizar tudo em branco e dois tons excessivamente chapados.
- Não há contorno global grosso. O amarelo em 2 é seletivo. Highlight de cabelo, rim e DOF exatos não podem ser separados de luz, compressão e desfoque da captura; usar efeitos discretos e validar em movimento.

## Primeiro experimento: só grama

Preparar cópias de master, instâncias, grass types e, se necessário, meshes/texturas dentro de `/Game/HypeWorld/VisualLab/Grama`. Preservar a cadeia e valores de MPC_Wind, animações, layout e chão/base RVT. Manter 25–35% de espaços como critério, não prometer que densidade fixa basta.

Sequência A/B: (1) baseline realmente Médio, saída e câmera fixas; (2) somente resolução/TSR em runtime, sem salvar configuração global; (3) largura real das lâminas, variedade moderada de altura e menos elementos; (4) macrocor compartilhada pelo terreno/RVT e pontas; (5) transmissão leve e mip alpha. Não misturar sombras, árvores, LUT ou personagem nessa comparação.

Para saída 1080p, FOV horizontal 90° e distância 10 m, 2–3 pixels na saída exigem aproximadamente 2,1–3,1 cm de largura projetada; a 67% interno, 2–3 pixels internos exigem 3,1–4,7 cm. São exemplos geométricos, não medidas da câmera atual. Lâminas inclinadas exigem margem. Aumentar só o clump XY não garante aumentar a largura do recorte alpha.

Verificar `Scale Mips for Alpha Coverage`, limiar da máscara, LOD e mip realmente usados antes de modificar. Preservar coverage indiscriminadamente pode engrossar o campo distante. Cull 30–40 m já aprovado; validar transição sem chuvisco e sem recolocar PDO dither, que foi recusado. Two Sided Foliage/transmissão equivalente com Substrate é candidato, não requisito de desempenho já comprovado.

## Opções para árvores — nenhuma troca realizada

| Opção | Benefício | Custo/risco |
|---|---|---|
| Copiar copas existentes; normais orientadas por volume arredondado e textura de massa de folhas | Menor retrabalho, preserva troncos e composição; melhora volume pintado | Normais não consertam silhueta espinhosa; cards extras aumentam overdraw e sombra |
| Remodelar só copas em massas arredondadas com LODs | Melhor controle da silhueta e leitura perto/longe; opção preferida | Trabalho de arte/UV/LOD, validação de sombra; substituição exige seu OK |
| Escala maior das copas | Prova visual rápida | Não elimina recortes, muda oclusão e sombras; pode invadir passagens; não aplicar no mapa sem autorização |
| Outro pack estilizado | Pode trazer silhueta e textura prontas | Licença/preço, consistência, materiais e LODs desconhecidos; escolher candidatos e custo antes de compra/troca |

## Toon para personagens — proposta, não implementação

Recomendo começar com materiais específicos por região (pele, tecido, cabelo), mantendo iluminação e sombras nativas, com um passe seletivo suave de duas faixas no stencil apenas se necessário. Pele com contraste muito baixo; tecido preserva normais/dobras; cabelo usa máscara de highlight; rim fraco; contorno desligado inicialmente. Primeiro inspecionar e neutralizar, em cópia, o PP existente para não empilhar quantizações. O projeto tem Substrate ligado: protótipo precisa validar acesso aos buffers e posição do passe no pipeline 5.8.

| Técnica | Custo e qualidade |
|---|---|
| Materiais por região + PP seletivo suave (recomendada para teste) | Mantém sombras do mundo; custo de material e, se usado, um passe fullscreen mais custom depth. Stencil não torna automaticamente o passe proporcional só ao personagem. Orçamento proposto: delta total ≤0,5 ms, não medido |
| Unlit com rampa N·L e direção do sol via parâmetro | Controle artístico alto e poucos cálculos, mas não recebe automaticamente sombras reais/luzes locais; integração adicional. Bom para preview isolado, menos adequado ao mundo atual |
| Shading model toon próprio na engine | Controle direto das faixas e sombras; manutenção/build de engine e compatibilidade com Substrate/5.8. Custo de engenharia alto; GPU só após protótipo |

Nenhum valor de ms é estimável com honestidade só pelo screenshot. A escolha visual é a primeira opção; aprovação e medição vêm antes de adotá-la.

O PP atual foi inspecionado: Mistura=1, Limiar=0,7, Suave=0,2, Sombra=0,6, Luz=1,25, RimForca=0,2, Espessura=1,5, ContornoOpacidade=0,9. Esses parâmetros ajudam a explicar por que uma túnica clara pode perder volume, mas exigem A/B com o PP desativado antes de concluir causalidade. O contorno atual também merece redução para atender ao novo alvo.

Pés: atlas/material, AO/sombra, interseção de grama e efeitos de chakra continuam hipóteses. Há componentes de chakra específicos para pés e alteração de integração em `e3f369d`; o script liga stencil no Indra, portanto não há fundamento para afirmar que ele perdeu stencil. Isolar cada contribuição por toggles temporários e capturar os pés em terreno sem grama, sem mover o mapa nem editar animações.

## Desempenho — sem benchmark novo

| Estado | Preset/saída | GPU | FPS / 1% low | Delta |
|---|---|---|---|---|
| Registro histórico `16d3ef8`, GTX 1650 | Médio, 1080p | 12,5–13,9 ms | 69,6–77 / 62–68 | Não comparável diretamente ao estado atual |
| Teste histórico CSM 4×2048 | Médio | +1,27 ms em ShadowDepths | Não registrado aqui | **Acima de 0,5 ms**; permanece desligado |
| Atual / bloco 0 | Não executado com renderização | Não medido | Não medido | Não medido; nenhuma mudança gráfica |

Em cada bloco futuro: mesma GPU, build, câmera, saída, preset completo Médio, VSync/frame cap documentados, aquecimento de shaders/streaming; no mínimo três capturas de 30 s incluindo câmera parada e movimento reproduzível. Registrar frame/GPU mediano e p95, FPS e 1% low, passes BasePass/ShadowDepths/TSR/PP. Avisar delta >0,5 ms. Não apresentar fotomontagem como depois renderizado. Só após aprovação atualizar Visual_Aprovado.md e fazer commit restrito aos arquivos do bloco.

## Referências técnicas

- Epic 5.8: [TSR](https://dev.epicgames.com/documentation/unreal-engine/temporal-super-resolution-in-unreal-engine?application_version=5.8).
- Epic 5.8: [Shading models](https://dev.epicgames.com/documentation/unreal-engine/shading-models-in-unreal-engine?application_version=5.8), transmissão Two Sided Foliage.
- Epic 5.8: [Substrate](https://dev.epicgames.com/documentation/unreal-engine/overview-of-substrate-materials-in-unreal-engine?application_version=5.8).
- Epic 5.8: [Post-process materials](https://dev.epicgames.com/documentation/unreal-engine/post-process-materials-in-unreal-engine?application_version=5.8).
