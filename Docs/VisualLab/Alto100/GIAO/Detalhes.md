# Protocolo GI/AO — 26/10/2026

GTX 1650, UE 5.8.4 Development `-game`, DX12, mapa nativo copiado `/Game/HypeWorld/VisualLab/Grama/L_Grama_Baseline`. Saída 1920×1080, screen percentage 100, resolução dinâmica desligada, sem VSync/limite de FPS. TSR qualidade 1, Flickering=0, histórico 100. Base Alto nos demais grupos, foliage e grass 0,8.

Câmera fixa igual ao bloco de sombras: posição (0; -312,028006; 250,352636), pitch -15°, yaw 90°, FOV 90°. Esta câmera passa a ser o protocolo comum; os percursos anteriores de AA não são diretamente intercambiáveis com estes números.

Sombras provisórias: 3 cascatas, MaxCSMResolution 1536; alcance da luz 5000 cm × DistanceScale 0,85 = 42,5 m; RadiusThreshold 0,06; mesmas 19 exclusões de caster do candidato anterior. Geometria permanece visível. PCSS e Distance Field Shadows preservados. Nenhum material modificado neste bloco, inclusive grama e arbustos.

Cada opção recebe 30 s de aquecimento, depois 3 capturas de 30 s com 5 s de intervalo. FPS = inverso do frametime médio; 1% low = inverso da média do 1% de frametimes mais lentos; GPU e passes = medianas. Captura PNG normal do jogo fora da janela de benchmark. CSVs brutos copiados para esta pasta.

Comparação controlada: grupo GI permanece Alto; muda apenas final gather (1 = Screen Probe Gather; 0 = Irradiance Field Gather), ou GI NONE no PostProcessVolume e DiffuseIndirect.Allow=0 para Sky Light + DFAO. A luz de céu existente é Movable, intensidade 3,1, cast shadows ativo; DFAO=1. Reflexos continuam Screen Space. Luzes, exposição, LUT, layout, personagens e vento não mudam.

Nomes confirmados no código local da engine: `Renderer/Private/Lumen/Lumen.cpp`, CVar `r.Lumen.FinalGatherMethod`, descrição dos métodos 0/1. A opção Irradiance mantém os demais parâmetros Alto para isolar o método; não representa trocar todo o preset para Médio.

Todas as modificações são temporárias no processo. Não salvar mapa, presets ou assets originais. Sem commit até aprovação.

## Verificação de execução

O ProfileGPU posterior às medições confirma `DistanceFieldLighting / ConeTraceGlobal / CombineCones / UpdateHistory`, dentro de SkyLightDiffuse, na opção DFAO. Os CSVs registram LumenScreenProbeGather no controle e LumenIrradianceFieldGather na alternativa. A CVar global de método GI é 0, mas o PostProcessVolume sobrescreve o método: o estado efetivo do volume e os passes executados são a evidência relevante.

SSAO de Alto foi preservado: ao desativar Lumen, ele aparece como custo adicional (~2,09 ms mais setup), não sendo correto atribuir todo o caminho sem Lumen ao DFAO. Medianas dos passes não devem ser somadas como se representassem um frame exato.

Não há validação de um percurso extenso, interiores, desoclusões ou transições de clipmaps neste bloco estático. A conclusão visual é limitada ao enquadramento comparado. Não extrapolar estes FPS a todo o mapa.
