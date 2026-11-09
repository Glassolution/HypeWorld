---
status: aprovado
atualizado: 2026-11-08
---
# Customização e catálogo de roupas

> O catálogo de clãs é a referência das opções; não criar posições vazias ou novos clãs.
> A roupa completa atual usa o rig Silent Stance mesmo quando há nomes históricos Indra.

Catálogo único: `UE/Source/HypeWorld/Ninja/HypeClanOutfitCatalog.inl`; UI e validação devem usar `GetOutfitsForClan` e os mesmos filtros.

| Posição Uchiha | ID preservado | Rótulo |
|---|---|---|
| 1 | `Outfit_NinjaHeadTest` | Sasuke pequeno |
| 2 | `Outfit_UchihaRobe` | Manto Uchiha |
| 3 | `Outfit_ShadowShinobi` | Shinobi das Sombras |

Cada clã tem posições 1–3; zero é reservado a roupa geral. Não ativar outros clãs sem pedido. O soft path atual do Manto aponta para `Clothes/CavaloMCP/SK_Uchiha_Robe` sob `/Game/HypeWorld/Personagens/SilentStancePreview`; alterações futuras exigem prova de carregamento automático.

Novas roupas recebem novos IDs/assets; revisão autorizada da mesma roupa pode integrar cópia própria mantendo seu ID, preservando a fonte e a versão anterior. Não recolocar `Outfit_SilentNinja` aposentada.

Fluxo aprovado: roupa completa composta; Top/Bottom/Shoes independentes não estão habilitados. `SetSkeletalMesh(VisualMesh, false)` preserva a instância de animação; manter componente, cápsula, fórmula de altura e overrides coerentes. Não assar a altura do jogador na malha. Cabelos seguem componente separado/Leader Pose.

Identidade/rede: conferir `Docs/Personalizacao.md` e código atual antes de alterar fluxo do criador. Relatos de outubro sobre catálogo Indra/286 ossos não descrevem a base atual do Silent Stance.

Prova de troca: mesmo personagem/câmera, preservar cabelo, cor, altura e cápsula, testar seleção no criador e carregamento ao reabrir. Fonte: catálogo atual lido nesta auditoria, guia da etapa 17 e prova da integração do Cavalo.

Relacionadas: [[Rig]], [[Legado-Indra]], [[Importacao]], [[Katon]].
