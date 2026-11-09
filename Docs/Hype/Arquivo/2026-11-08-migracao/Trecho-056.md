---
status: arquivado
atualizado: 2026-11-08
---
# Roupas por clã — 05/11/2026 — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Roupas por clã — 05/11/2026

Cada clã terá três roupas exclusivas, registradas nas posições 1 a 3. O catálogo único fica em `Source/HypeWorld/Ninja/HypeClanOutfitCatalog.inl` do projeto Unreal. `RequiredClan` define o clã e `ClanOutfitSlot` a posição; zero é reservado para roupas gerais. UI e validação de equipamento usam `GetOutfitsForClan`, com checagem de skeleton, ID e posição duplicada. Não inventar um terceiro modelo nem exibir opção vazia.

Uchiha: `Outfit_NinjaHeadTest` conserva seu ID salvo e aparece como **Sasuke pequeno** (posição 1); `Outfit_UchihaRobe` é **Manto Uchiha** (posição 2). A posição 3 é **Shinobi das Sombras** (`Outfit_ShadowShinobi`), com fontes e validação na etapa 33. Os outros clãs não foram ativados por esta integração.

O manto é um novo visual completo em `Clothes/UchihaRobe/SK_Uchiha_Robe`, não uma substituição da base ou da Ninja. Fontes, Blender, FBX, backup e validações ficam na etapa 19. Para extrair cabeça e mãos, preferir as superfícies já aprovadas do `SK_Ninja_Fitted` na etapa 18, preservando seus pesos normalizados e corretivos. Classificação de pele por cor sozinha pode apagar olhos e unhas ou deixar fragmentos de camiseta na nuca; conferir a atribuição de material e a geometria. Não alongar o pescoço a partir de um corte que atravesse a mandíbula.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
