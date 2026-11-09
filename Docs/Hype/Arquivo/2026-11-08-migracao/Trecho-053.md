---
status: arquivado
atualizado: 2026-11-08
---
# Integração com a personalização — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## Integração com a personalização

O fluxo aprovado hoje é **roupa completa composta**, no mesmo componente visível `Indra` do personagem, embora o visual seja Silent Stance. O nome histórico do componente não significa que se deva voltar ao skeleton do Indra.

Registrar um novo `OutfitId`, rótulo e mesh compatível em `HypeAppearanceComponent`. O seletor lê `OutfitOptions` e filtra skeleton e clã. Não sobrescrever `SK_Ninja_HeadTest` nem `Outfit_NinjaHeadTest` para adicionar outro traje. Não recolocar a opção aposentada `Outfit_SilentNinja`.

Preservar o comportamento atual: troca com `SetSkeletalMesh(VisualMesh, false)`, manutenção da instância de animação, limpeza de overrides de material ao trocar de mesh, mesmo componente, cápsula e fórmula de altura. A escala observada na altura de 163 cm é 0,90564098 e a cápsula é raio 35 cm / meia-altura 90 cm; a escala é calculada pelo sistema de altura, não deve ser assada de novo na roupa. Cabelos continuam em componente separado com Leader Pose e cores independentes dos acessórios.

Peças avulsas Top/Bottom/Shoes não estão habilitadas por esse caminho do Silent Stance: a implementação atual as oculta. Não afirmar que camisas/calças podem ser combinadas livremente sem implementar e validar essa funcionalidade.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
