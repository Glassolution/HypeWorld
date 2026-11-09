---
status: arquivado
atualizado: 2026-11-08
---
# Referência atual e arquivos de partida — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Referência atual e arquivos de partida

A referência atual é o **Silent Stance aprovado**, não o Indra histórico nem o Manny bruto. O Indra e seus assets continuam preservados. Instruções antigas que indicam o Indra como base de novas roupas foram superadas pelo pedido posterior do usuário para usar o Silent Stance.

Workspace: `/Users/luisfelipexavier/Desktop/Hype World`.
Projeto executado: `/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld/HypeWorld.uproject`.
Assets base: `/Game/HypeWorld/Personagens/SilentStancePreview`.

| Referência | Fonte aprovada |
|---|---|
| Corpo, rig, mãos e corretivos | `Silent Stance/08 - Mãos e dedos/Silent Stance - mãos.blend` |
| Roupa final, ombros suaves, gola alta e acabamento | `Silent Stance/14 - Acabamento clean/Ninja - acabamento clean.blend` |
| Cabelos aprovados, originais com escala proporcional | `Silent Stance/16 - Cabelos originais proporcionais/Cabelos originais - Silent Stance.blend` |
| Transformações dos 161 ossos no Unreal | `Silent Stance/08 - Mãos e dedos/referência final Unreal.json` |
| Hierarquia/matrizes de referência do Blender | `Silent Stance/10 - Caimento da roupa/rig aprovado.json` |
| Poses de comparação | `Silent Stance/09 - Roupa ninja/poses de referência aprovadas.json` |
| Cópias congeladas e hashes | `Referências congeladas/` e `referencias_aprovadas.json`, nesta pasta |

No Blender, os nomes são `Silent_Stance_Rig` (rig), `Silent_Stance_LOD0` (base) e `SK_Ninja_Fitted` (visual composto final). Há cópias intermediárias na cena; não exportar todos os objetos visíveis indiscriminadamente.

No Unreal:

- Corpo: `SK_SilentStance_Preview`; skeleton: `SK_SilentStance_Preview_Skeleton`.
- Animação: `ABP_SilentStance_Preview`, com a lógica nativa de `SilentStanceAnimInstance.cpp`.
- IK/retarget: `IK_SilentStance_Preview`, `IK_Manequim_Preview`, `RTG_Manequim_SilentStance_Preview`. Preservar também o `RTG_Indra_SilentStance` existente.
- Roupa aprovada: `/Game/HypeWorld/Personagens/SilentStancePreview/Clothes/NinjaHeadTest/SK_Ninja_HeadTest`, ID `Outfit_NinjaHeadTest`, rótulo `Ninja (novo)`.
- Poses de mão finais em `HandValidation`: `PO_Selo_DoisDedos` e `PO_Punho`. O arquivo de relatório diferencia esses assets finais dos testes exploratórios.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
