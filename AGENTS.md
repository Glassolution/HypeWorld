# Hype World — padrão de personagens

## Referência atual: Silent Stance aprovado

Por instruções posteriores do usuário em 04/11/2026, novas roupas e personalização usam o **Silent Stance aprovado**, com os ombros, mãos e proporções já corrigidos. Em seguida o usuário pediu para registrar e reutilizar esse processo, evitando repetir as mesmas correções.

Antes de mexer em modelos, ler:
`/Users/luisfelipexavier/Desktop/Hype World/Silent Stance/17 - Padrão aprovado para novas roupas/PADRAO_PERSONAGENS.md`.

Skill correspondente: `hype-world-personagens`. Cópias congeladas, hashes e verificador ficam na mesma pasta do guia.

- Roupa final: etapa 14, `Ninja - acabamento clean.blend`, com o refinamento de gola da etapa 18 registrado no guia; mãos: etapa 08; cabelos originais: etapa 16. Rascunhos e versões rejeitadas não são referências aprovadas.
- Reutilizar os 161 ossos, hierarquia, bind pose, pesos/corretivos das mãos, twists, ABP e retargeting aprovados. Ajustar roupa à anatomia; não mover ombros/falanges ou engrossar o corpo para fazê-la caber.
- Preservar a base e a roupa Ninja aprovadas. Criar novos assets/IDs para outras roupas. Backup antes de editar malha ou pesos.
- Cabelos: GLBs originais, escala uniforme e posição; não acumular deformações nas cópias do Indra ou da etapa 15. Respeitar o material OPAQUE da fonte.
- Conferir no jogo, com câmera fixa e referência ao lado, antes de entregar: proporções, materiais, idle/andar/correr, ombros, gola/nuca, mãos, mangas e troca no criador. Resolver autonomamente os ajustes rotineiros registrados no guia.
- Não alterar jutsus, VFX ou mapas por causa de uma roupa. Preservar modelos anteriores.

## Indra: manutenção do modo legado

O padrão histórico do Indra permanece válido para o próprio Indra e para manutenção explícita do modo legado, não como molde para novas roupas do Silent Stance.

- Fonte: `Content/Python/indra_character_standard.json`. Não recapturar esse perfil de outro modelo.
- Preservar `SKEL_Indra`, hierarquia, bind pose, proporções, animações originais e a escala visual 1,06 do modo legado. Sua cápsula e velocidades são as registradas no perfil.
- No fluxo legado, preparar em cópia, vincular ao rig do Indra e conferir compatibilidade real; não simular conversão trocando apenas a propriedade Skeleton.
- `indra_character_standard.apply_mesh(mesh)` e o menu Tools > Hype World referem-se ao fluxo legado. Não usá-los para substituir o rig aprovado do Silent Stance.
- Instruções históricas: `Docs/PadraoPersonagem/README.md`, `Indra_Referencia.fbx`.
