# Sombras: diagnóstico e testes

Tudo em runtime na cópia VisualLab; geometria, layout, vento e animações preservados. Nenhuma configuração global ou asset salvo. Alto a 1080p nativo, com AA provisório TSR Q1 após o bloco 1b não eliminar o ghosting do TAA. Câmera parada no enquadramento original da baseline; não comparar FPS diretamente com o percurso móvel do AA.

## Custo por tipo

Triagem de 30 s por grupo, 10 s de estabilização, alterando apenas cast_shadow dos componentes; todos os demais grupos restaurados antes de cada caso. Os objetos continuam visíveis. Esta triagem não é benchmark final de uma configuração proposta; os candidatos de cascatas recebem 3×30 s cada.

| Grupo com sombras removidas temporariamente | ShadowDepths | Redução marginal |
|---|---:|---:|
| Nenhum (controle) | 7,485 ms | — |
| Rochas | 7,386 ms | 0,099 ms |
| Árvores/troncos | 3,768 ms | **3,717 ms** |
| Arbustos/flores | 6,846 ms | **0,639 ms** |
| Personagens | 7,452 ms | 0,033 ms |
| Outros: terreno, estruturas e props | 7,029 ms | 0,456 ms |

As reduções não são parcelas aditivas: existem custos compartilhados do passe, culling e rasterização. Não atribuir a diferença restante a um objeto sem medir. O frame ProfileGPU original registrou Nanite Shadows em 7,026 ms dentro de ShadowDepths 7,777 ms; é amostra pontual, não a mediana da triagem.

Árvores do MegaKit (CommonTree, TwistedTree, DeadTree) possuem dados Nanite confirmados via get_num_nanite_triangles, assim como Bush_Common/Bush_Common_Flowers. As 193 componentes de SM_Stone_Boulder_1 do StyleHex não retornam triângulos Nanite; rochas MegaKit retornam. Contagem de componentes não equivale a contagem de instâncias de foliage. Inventários completos estão em inventario.json, grupos.json e nanite_dados.json.

Correção de classificação: três meshes de água criados em runtime continham '/Grama/' no caminho do mapa e entraram inicialmente no rótulo grama; não são lâminas de grama. A classificação final usa o nome do mesh. Grama real não tem cast_shadow dinâmico ativo; não há um custo de sombra da grama a cortar. O grupo “outros” da triagem acima não inclui esses três meshes de água. Registro em classificacao_corrigida.json.

## Candidatos

1. Original: quatro cascatas, máximo CSM 2048, distância da luz 8000 cm.
2. Cortes pequenos: sombras de arbustos/flores desligadas; r.Shadow.RadiusThreshold=0,06 (original 0,04); props com raio de bounds menor que 100 cm deixam de projetar sombra além de 1500 cm da câmera. É um corte de sombra aplicado em runtime, sem usar Max Draw Distance da geometria e sem ocultar os objetos. Não é uma nova propriedade de asset ou sistema persistente de culling.
3. Mesmos cortes + três cascatas, máximo CSM 1536, distância da luz 5000 cm.
4. Mesmos cortes + duas cascatas, máximo CSM 2048, distância da luz 5000 cm.

Os fatores de escala da engine permanecem registrados em *_config.json; a distância efetiva é afetada por r.Shadow.DistanceScale. PCSS original, Source Angle 2,5°, Distance Field Shadows e alcance DF 51200 cm preservados. Sombras das árvores grandes permanecem nos candidatos; sua remoção anterior foi só diagnóstico. Nenhum VSM ativado.

Benchmark final: 25 s de estabilização por caso, 3×30 s, intervalo de 5 s; nenhum vídeo/encoder durante a medição. FPS médio=1000/média de FrameTime, 1% low=1000/média do 1% mais lento, GPU/passes medianos. Prints nativos após as medições, sem retoque. CSVs, scripts e estados ficam nesta pasta.

O primeiro rascunho de candidato usava threshold 0,03; ao conferir o controle efetivo 0,04, foi corrigido para 0,06. A coleta dos candidatos foi reiniciada após a correção, preservando apenas o controle original completo. Não usar a captura parcial pré-correção; o status dessa preparação está em status_pre_correcao_threshold.json.
