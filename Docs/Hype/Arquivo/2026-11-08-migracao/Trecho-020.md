---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
- Velocidades x animação: BS_Ninja_Locomocao: parado 0 (MM_Idle, todas as direções), andar Walk2 (169 frente/164 lados/107 costas), correr Run2 (560 frente/428 esquerda/342 direita). Corrida ninja = mesma animação Run2_Forward acelerada + pose (tronco deitado 40° a partir do quadril — as coxas desfazem o giro do quadril —, quadril 12 cm mais baixo com joelhos dobrados, braços esticados para trás um pouco acima da horizontal (SprintArmsDrop -0,35), retos e paralelos (SprintArmsInward 0), palmas para cima (o braço gira no próprio eixo, o punho não torce), dedos retos (SprintHandRelax 0,8), pés apontando para a frente (SprintFootStraight 0,8). A cada pisada (pé mais baixo = pé de apoio) o quadril e o tronco balançam para o pé de apoio (SprintStepSway 5°); as mãos seguem só o balanço de lado do peito (a torção do peito da animação é desfeita em 60%) e flutuam atrasadas no balanço e no sobe e desce (SprintHandFloat 1)) no NinjaAnimInstance e o personagem acelera a animação pela curva AnimSpeedCurve (velocidade do jogo → velocidade natural da animação). Ao trocar amostras por script, chamar `unreal.NinjaEditorTools.rebuild_blend_space(bs)` (senão o blend space fica parado) e desligar snap_to_grid. Linha extra de andar 4 direções (Walk2_Forward/Left/Right/Backward em 169/164/164/107). O ABP do template limitava a Direction a ±45°: o C++ (NinjaAnimInstance::NativeThreadSafeUpdateAnimation) sobrescreve com a direção completa. Velocidade natural de uma animação = deslocamento do root ÷ duração (não usar a velocidade do pé sem o root: dá errado).
- Animações de movimento (só com animações do manequim): montagens em `/Game/Ninja/Characters/Animations/` tocadas pelo personagem ao mudar de modo; inclinação e velocidade da animação calculadas no C++ (`UpdateLean`, `UpdateRunAnimRate`). Faltam animações próprias de corrida na parede, corrida ninja e virar no lugar (vir de fora: Higgsfield/Fab).
- Pose procedural (`Source/HypeWorld/Ninja/NinjaAnimInstance`): retoca a pose depois do gráfico do `ABP_Ninja` (mão na parede, olhar o chão/encolher pernas antes de pousar, agachar no pouso). Ajustes em Class Defaults do ABP_Ninja, seção Ninja. Scripts não conseguem editar o gráfico de animação; mudanças de pose vão nesse C++.
- Animações do Mixamo: FBX em `Desktop/Hype World/Mixamo/` → `Tools/editor/importar_mixamo.py` (esqueleto do X Bot) → `Tools/editor/retarget_mixamo.py` (IK_XBot → IK_Manequim; root motion gerado do quadril e colado no chão; Pin Bones para ik_foot/ik_hand; dedos não mapeados) → `Tools/editor/criar_locomocao_mixamo.py` (BS_Ninja_Locomocao + troca dos nós do ABP_Ninja via export_text/import_text). Velocidade de andar/correr = velocidade do pé de apoio da animação (medir antes de mudar).
- **Animações novas (dash/pulo/corrida iguais ao vídeo de referência) — plano em andamento:** as acrobacias feitas por código ficaram ruins (Felipe reprovou; DashFlipDegrees e JumpSpinDegrees zerados no BP — o código continua lá). Caminho: Higgsfield gera um vídeo realista (pessoa real, de lado, corpo inteiro, roupa justa, câmera parada, 1080p+, 30 fps) → DeepMotion Animate 3D (conta Freemium do Felipe: 25 créditos/mês + bônus, clipes até 20 s, 30 fps; ~2 créditos por 1,2 s) converte em FBX → `Mixamo/Captura/` → converter para o manequim como as do Mixamo. O vídeo de gameplay foi RECUSADO pelo DeepMotion (personagem de jogo, casaco cobrindo o corpo, câmera em movimento). Trechos recortados do vídeo de referência: `Referencias/Videos/ref_*.mp4`; vídeo original: `~/Downloads/ScreenRecording_10-23-2026 22-50-53_1.mov`. Plano B: SayMotion (texto → animação, mesma conta DeepMotion). Regra: antes de gerar no Higgsfield, informar modelo e custo e esperar aprovação.

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
