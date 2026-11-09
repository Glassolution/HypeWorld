---
status: arquivado
atualizado: 2026-11-08
---
# 3. Por que estamos fazendo isso — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## 3. Por que estamos fazendo isso
Felipe mandou um vídeo de referência (gameplay de outro jogo ninja) com a corrida, o dash e o pulo que ele quer. Eu tentei imitar **por código** (girando o corpo inteiro em cima da animação de dash antiga). Ele reprovou: "péssimas, nada a ver com o vídeo". Conclusão: precisa de **animação de verdade** (captura de movimento), não pose por código.
- As acrobacias por código foram **desligadas** (BP_ThirdPersonCharacter: `DashFlipDegrees = 0`, `JumpSpinDegrees = 0`; o código `UpdateAcrobatics` continua no personagem). A inclinação forte nas curvas (`MaxTurnLean = 32`) continua ligada.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
