---
status: arquivado
atualizado: 2026-11-08
---
# ARQUITETURA E PADRÕES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## ARQUITETURA E PADRÕES

- **Projeto em C++** com Blueprints por cima. A lógica central fica em C++; ajustes de design e tuning ficam em Blueprint.
- **Gameplay Ability System (GAS)** para jutsus, combos, chakra, vida, cooldowns e buffs. Cada jutsu é uma Gameplay Ability; chakra e vida são Attributes.
- **Enhanced Input** para os controles.
- **Replicação desde o início.** Todo sistema novo nasce pronto para multiplayer: o servidor é a autoridade, com predição no cliente onde o GAS suporta. Nunca criar um sistema "só single-player para adaptar depois".
- **Niagara** para todos os VFX. Vídeo do Higgsfield **não** é VFX de jogo. O Higgsfield entra só como gerador de texturas e flipbooks usados dentro do Niagara.
- **Animações:** Animation Blueprint + Montages com Anim Notifies para as janelas de dano, cancelamento e disparo de jutsu.
- **Fluxo de assets:** Higgsfield gera → Blender limpa, faz retarget para o esqueleto da UE5 e exporta FBX → Unreal importa.
- **Controle de versão:** Git + Git LFS para assets binários (.uasset, .umap, .fbx, texturas).
- **Nomenclatura:** prefixos padrão da Unreal: `BP_`, `ABP_`, `AM_`, `GA_`, `GE_`, `NS_`, `M_`, `MI_`, `T_`, `SK_`, `SM_`.
- **Pastas:** `/Content/Ninja/` subdividida em `Characters/`, `Abilities/`, `VFX/`, `Maps/`, `UI/`, `Core/`.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
