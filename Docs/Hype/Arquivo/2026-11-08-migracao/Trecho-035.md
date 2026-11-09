---
status: arquivado
atualizado: 2026-11-08
---
# Indra: manutenção do modo legado — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Indra: manutenção do modo legado

O padrão histórico do Indra permanece válido para o próprio Indra e para manutenção explícita do modo legado, não como molde para novas roupas do Silent Stance.

- Fonte: `Content/Python/indra_character_standard.json`. Não recapturar esse perfil de outro modelo.
- Preservar `SKEL_Indra`, hierarquia, bind pose, proporções, animações originais e a escala visual 1,06 do modo legado. Sua cápsula e velocidades são as registradas no perfil.
- No fluxo legado, preparar em cópia, vincular ao rig do Indra e conferir compatibilidade real; não simular conversão trocando apenas a propriedade Skeleton.
- `indra_character_standard.apply_mesh(mesh)` e o menu Tools > Hype World referem-se ao fluxo legado. Não usá-los para substituir o rig aprovado do Silent Stance.
- Instruções históricas: `Docs/PadraoPersonagem/README.md`, `Indra_Referencia.fbx`.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
