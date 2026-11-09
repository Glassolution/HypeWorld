---
status: legado-preservado
atualizado: 2026-11-08
---
# Indra: somente manutenção do legado

> Os assets históricos do Indra continuam preservados, mas não são o molde das roupas atuais.
> Não trocar a propriedade Skeleton para simular uma conversão entre rig legado e Silent Stance.

- Perfil legado: `UE/Content/Python/indra_character_standard.json`; não recapturá-lo de outro modelo.
- Preservar `SKEL_Indra`, hierarquia/bind, proporções, animações, escala visual 1,06 e cápsula/velocidades registradas no próprio perfil.
- `indra_character_standard.apply_mesh` e o menu Tools > Hype World desse fluxo pertencem ao legado. Não aplicá-los para substituir o rig aprovado atual.
- Referências históricas: `UE/Docs/PadraoPersonagem/README.md`, `Indra_Referencia.fbx`; abrir só no escopo explícito de manutenção do Indra.
- O nome de um componente visível pode continuar `Indra` por compatibilidade. Isso não autoriza voltar aos ossos antigos.

Fonte: AGENTS anterior do UE, preservado integralmente no arquivo da migração.

Relacionadas: [[Rig]], [[Customizacao]], [[Arquivo/2026-11-08-migracao/Indice]].
