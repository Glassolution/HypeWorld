---
status: arquivado
atualizado: 2026-11-08
---
# Hype World — referência aprovada de personagens — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
# Hype World — referência aprovada de personagens

O usuário pediu para guardar e reutilizar o procedimento aprovado de rig, mãos, texturas e dimensionamento ao adicionar mais roupas. Antes de trabalhar em personagens/personalização, ler:

`Silent Stance/17 - Padrão aprovado para novas roupas/PADRAO_PERSONAGENS.md`

Skill reutilizável: `hype-world-personagens` (cópia em `Silent Stance/17 - Padrão aprovado para novas roupas/Skill/hype-world-personagens/`).

- A referência atual é o **Silent Stance aprovado**, não o Indra histórico. Preservar também os assets legados do Indra.
- Pontos de partida: roupa final da etapa 14 com o refinamento de gola da etapa 18, mãos da etapa 08, cabelos originais da etapa 16; cópias congeladas e hashes na etapa 17.
- Reutilizar os 161 ossos, bind pose, pesos/corretivos de mãos, twists, ABP e retarget aprovados. Ajustar a roupa ao corpo; não alterar o corpo para fazê-la caber.
- Não partir de Rascunhos, Rejeitado, antigas roupas largas ou cabelos já deformados. Cabelos novos: fonte original + escala uniforme + posição.
- Backup antes de editar pesos/malha. Novas roupas recebem novos assets/IDs; não sobrescrever base ou roupa aprovada.
- Conferir autonomamente proporções, materiais, deformações e troca no criador antes de entregar, com screenshots de câmera fixa no jogo. O usuário não deve precisar repetir as correções já registradas.
- Projeto Unreal executado: `/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld`; este workspace guarda fontes, referências e procedimentos.

- Roupas exclusivas: cada clã tem três posições (1–3). Uchiha: posição 1 `Outfit_NinjaHeadTest` / Sasuke pequeno; posição 2 `Outfit_UchihaRobe` / Manto Uchiha; posição 3 `Outfit_ShadowShinobi` / Shinobi das Sombras. Usar o catálogo `Source/HypeWorld/Ninja/HypeClanOutfitCatalog.inl` e o mesmo filtro `GetOutfitsForClan` no criador e na validação. Não ativar outros clãs nem criar opções vazias por causa de uma roupa.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
