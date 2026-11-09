---
status: arquivado
atualizado: 2026-11-08
---
# Personalização de personagem (29/10/2026) — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
- `Ninja/HypeAppearanceTypes.h`, `UHypeAppearanceComponent` no personagem e `FHypeAppearance` no save. Prévia local; confirmação e perfil carregado passam pelo servidor, com limites para altura/cores e IDs filtrados pelo catálogo; estado replicado. Corpo `SK_Base_Body` já existente, validado nos 286 ossos do padrão Indra; material `MI_Base_Body` com `BaseColor Tint` dinâmico. Escala uniforme só na instância visível; cápsula, rig, animações e referência 1,06 preservados.
- Sem modelos novos de cabelo/roupa: opções informativas e paleta de cabelo desativada até selecionar um modelo. Futuras peças no componente Appearance → HairOptions/OutfitOptions, mesmo rig/bind pose. Não há morphs faciais, sexo ou tatuagens fictícios. Guia: `Docs/Personalizacao.md`.
- Validação: compilação `Result: Succeeded`; 3/3 testes `HypeWorld.Appearance` passaram (entrada inválida, save round trip, assets compatíveis). Prévia PIE conferida: corpo base, pele, altura 165 cm com escala 0,916753; navegação preserva nome e cancelar restaura malha/escala originais. Evidências em `Docs/Personalizacao/`. Partida com dois jogadores ainda não validada visualmente.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
