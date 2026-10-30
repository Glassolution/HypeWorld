# Cabelos e roupas Uchiha

41 modelos de cabelo e quatro trajes completos estão conectados à etapa Estilo do criador. As peças usam o corpo base e o rig do Indra. Não há alteração de cápsula, velocidades, animações, mapas ou VFX. Os arquivos originais permanecem preservados.

As roupas são Traje Shinobi Azul, Armadura Shinobi, Traje Sombra e Uniforme Tático. O catálogo marca `RequiredClan=Uchiha`; a interface filtra as opções e o componente valida os IDs depois da atribuição de clã pelo servidor. Cancelar restaura a aparência anterior. Cabelo e roupa são salvos e replicados pelo perfil existente.

## Preparação

- Fontes de cabelos: `SourceAssets/Customization/Hair`.
- Quatro ZIPs originais da pasta Downloads: `SourceAssets/Customization/Clothes`.
- FBXs ajustados, texturas e manifesto: `SourceAssets/Customization/Fitted`.
- Preparação reproduzível no Blender: `Tools/customization/fit_indra_cosmetics.py`.
- Importação reproduzível na Unreal: `Tools/editor/importar_cosmeticos_indra.py`.
- Catálogo de runtime: `Source/HypeWorld/Ninja/HypeFittedCatalog.inl`.

As mangas são alinhadas ao eixo ombro–punho medido no corpo base; as roupas têm aproximadamente 45 mil triângulos cada, com até quatro influências por vértice transferidas da superfície do corpo. Cabelos seguem o osso da cabeça. A geometria usa centímetros na exportação; o rig não é escalado para caber nas roupas. Os dois cabelos com origem diferente recebem ajuste individual de posição e largura.

A importação compara nomes, hierarquia e transformações dos 286 ossos, rejeita divergências grandes e restaura a referência canônica na cópia da malha antes da validação estrita. O FBX do Blender emite aviso de matrizes de bind relativas; o asset final é validado contra `indra_character_standard.json`. O esqueleto original não é regravado. Os materiais usam a função `SetMaterials` da UE 5.8 para atualizar também o cache persistente. A pele do criador usa uma instância própria em modo Masked, ocultando o corpo sob a roupa.

## Limites das fontes

Quatro GLBs são personagens completos sem cabelo separado por material: `naruto.glb`, `naruto__sasuke_low_poly__rig__texture.glb`, `naruto_sage.glb` e `naruto_shippuden-_naruto.glb`. Ficaram preservados, fora do catálogo, para extração posterior. `zabusa (1).glb` é uma cópia idêntica e não duplica a opção. Os cabelos compridos seguem rigidamente a cabeça; este passe não adiciona simulação física.

## Evidências

`import_validation.json` registra os 45 assets e seus limites reais na Unreal. `runtime_validation.json` registra os testes do criador e poses. O relatório de automação fica em `test-report/index.json`; as capturas desta pasta mostram as roupas e os cabelos no jogo. Os logs de compilação e o roteiro de verificação acompanham as evidências.

Última verificação: compilação aprovada e 6/6 testes automatizados aprovados. Guarda, corrida e trote lateral conferidos nas capturas. O clipe Tigre foi aplicado em runtime, mas a captura ficou encoberta pela copa de uma árvore; sua revisão visual permanece pendente.
