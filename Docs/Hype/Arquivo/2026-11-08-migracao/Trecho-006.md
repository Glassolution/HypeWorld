---
status: arquivado
atualizado: 2026-11-08
---
# Cabelos e roupas Uchiha (30/10/2026) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
### Cabelos e roupas Uchiha (30/10/2026)
- Catálogo do criador: 41 cabelos e quatro trajes completos, em `Content/Ninja/Characters/Customization`. Fontes ajustadas e quatro ZIPs originais em `SourceAssets/Customization/Fitted` e `Clothes`; originais de Downloads preservados.
- Roupas restritas a Uchiha (`RequiredClan`), filtradas no menu e validadas pelo servidor após atribuir clã. Materiais de cada cabelo são reiniciados ao trocar a malha. Corpo usa `MI_CreatorBody`, instância própria com máscara; sem alteração do corpo/rig/animações originais.
- Preparação Blender: `Tools/customization/fit_indra_cosmetics.py`; importação: `Tools/editor/importar_cosmeticos_indra.py`. Bind pose e escala dos 45 assets conferidos contra os 286 ossos canônicos. A UE 5.8 exige `SetMaterials` (atualiza cache persistente) e uso SkeletalMesh ativo no material/instâncias. Extração de textura sem transformação AgX.
- Quatro GLBs de personagens completos ficaram fora do catálogo para extração posterior; uma cópia idêntica de Zabuza não duplica a opção. Cabelos compridos não têm simulação física.
- Evidências de compilação, importação, materiais e testes em `Docs/Personalizacao/Trajes`.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
