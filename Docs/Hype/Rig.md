---
status: aprovado
atualizado: 2026-11-08
---
# Personagem e rig aprovados

> Silent Stance é a referência atual; Manny orienta o retarget, não substitui o bind aprovado.
> Ajustar a roupa à anatomia numa cópia, sem reconstruir a base ou repetir fontes rejeitadas.

- Preservar **161 ossos**, nomes, hierarquia, bind pose, pesos, twists, dez corretivos de dedos e ABP/retarget aprovados. O mesmo nome de Skeleton não prova compatibilidade.
- Fonte das mãos/corpo: `WS/Silent Stance/08 - Mãos e dedos/Silent Stance - mãos.blend`.
- Roupa: etapa 14 `Ninja - acabamento clean.blend`, com gola da etapa 18 `Ninja - gola maior e encorpada.blend`. Referências congeladas e hashes: etapa 17; consultar o manifesto/verificador só quando tocar a referência.
- Cabelos originais: etapa 16. Novos cabelos partem do GLB fonte, escala uniforme e posição. Não deformar cópias antigas do Indra ou da etapa 15.
- Na Unreal: `SK_SilentStance_Preview`, `SK_SilentStance_Preview_Skeleton`, `ABP_SilentStance_Preview`; IK/retarget existentes são o ponto de partida. Preservar os legados.
- Bases antigas, Rascunhos, Rejeitado e roupa larga superada não são fontes aprovadas. Não engrossar corpo/mãos ou mover o bind de ombros/falanges para acomodar roupa.
- Corretivos novos começam na Basis neutra (`from_mix=False`) e ficam restritos ao trecho/pose que os exige. Não transferir pesos/corretivos de dedos para tecido ou antebraço indiscriminadamente.
- Conferir proporções, ombros/gola/nuca, mãos/mangas, idle/andar/correr, gestos e troca no criador com câmera fixa. Para selos, incluir transições lentas e malha final; não validar só massa cinza.

Fonte detalhada preservada: documento `WS/PADRAO_PERSONAGENS.md` no manifesto do arquivo. O caminho antigo do guia é uma entrada curta; a skill `hype-world-personagens` continua disponível para execução especializada, sem exigir seu carregamento em tarefas não relacionadas.

Relacionadas: [[Importacao]], [[Materiais]], [[Customizacao]], [[Legado-Indra]], [[Selos/Carneiro]].
