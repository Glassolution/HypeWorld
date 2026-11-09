---
status: arquivado
atualizado: 2026-11-08
---
# PLANO POR FASES — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
- **Câmera e direção do corpo (atual, 24/10/2026 — pedido do Felipe: "câmera sempre nas costas enquanto se move"):** andando e trotando o corpo fica de frente para onde a câmera olha (`bWalkFacesCamera` = true, `StrafeMaxAngle` 180): D/A = passo lateral, S = de costas (trote para trás = `RunBackSpeed` 220); parado a câmera orbita livre e o corpo não gira. O pack não tem lados/costas: BS_ANAP_Locomocao usa Mixamo `Walk2_Left/Right/Backward` e `Run2_Left/Right` nessas direções (frente = pack); a taxa da animação mistura as velocidades naturais por direção (`WalkStrafeNatural` 164, `WalkBackNatural` 107, `RunStrafeLeft/RightNatural` 428/342). Corrida ninja: corpo vira para onde corre e a câmera sempre volta para as costas do corpo (`UpdateCameraFollow`, `CameraFollowSpeed` 4); curva: `SprintSteerAngle` 18 (giro ≈ ângulo × atrito do chão 8 ≈ 150–160°/s medido; 10 = 80°/s ficou lento no PvP, 35 = giro tonto), `SprintRotationRate` 420. Câmera mais alta: braço 360, SocketOffset Z 120, câmera inclinada −12°. Teste: `Tools/editor/teste_camera.py`.
- **Voo dos saltos especiais (atual, 24/10/2026 — Felipe: super pulo "como catapulta no espaço", mortal "mais alto, levitando"):** cada salto tem a própria gravidade (`UNinjaMovementComponent::StartFloatJump(Subida, Topo/queda, QuedaMáx)`, override de `GetGravityZ`; × GravityScale 2; acaba ao pousar; sem frenagem no ar `bInSuperJump`). **Super pulo:** `SuperJumpGravity` (0,35; 0,35; 900) = gravidade fraca o voo todo → arco baixo e longo, sai empurrado; `SuperJumpForwardSpeed` 700→1300, `SuperJumpUpSpeed` 650→1000. Medido cheio: 41 m, 7 m de altura, 3,1 s no ar. **Mortal para trás:** `BackflipGravity` (0,55; 0,15; 550) = sobe e fica ~1 s quase parado no topo; `BackflipUpSpeed` 1300, `BackflipBackSpeed` 240. Medido: 8,3 m de altura, ~9 m para trás, ~4,1 s no ar (antes 5,9 m / 2,1 s). Pulo normal não mudou. Valores do BP: `Tools/editor/ajustar_voo_planado.py`; teste `Tools/editor/teste_mortal.py`. Gotcha: mudar o CDO do BP por Python com o editor aberto não chegou no personagem do Play seguinte (valor velho) — para testar na hora, mudar também no personagem do Play; depois de reabrir o editor vale o salvo.
- **Fluidez no ar (24/10/2026, Felipe: "pulo duplo + dash freava"):** causas: o fim do dash trocava a velocidade pela de andar (150) e a engine freava no ar acima do limite (BrakingDecelerationFalling 1500). Agora (`NinjaMovementComponent`): dash sem animação sai forte e assenta (`DashStartBoost` 1,35 → `DashEndSpeedFraction` 0,85 × `DashSpeed` 1600 no BP) e nunca deixa mais lento que a entrada (`DashEntrySpeed`); fim do dash no ar = `AirDashExitSpeedFraction` 0,75 + `AirDashExitUpSpeed` 200 e `bAirMomentum` (sem frenagem até pousar); no chão sai a `GroundDashExitSpeedFraction` 0,45 e o atrito desacelera suave; no ar segurando direção NUNCA freia (`ApplyVelocityBraking`); pousar rápido (> RunSpeed) apertando direção liga a corrida ninja (`bLandIntoSprint`). Medido (pulo + pulo duplo + dash, W segurado): dash 21→15 m/s, sai a 12 m/s mantidos até o chão, pousa e segue a 8 m/s (corrida ninja), 26,7 m no total. **Super pulo pela mira:** inclinação da câmera `SuperJumpVerticalPitch` (10°→55°) mistura arco para a frente com salto reto (`SuperJumpVerticalSpeed` 1200→1900); `SuperJumpUpSpeed` 800→1300. Medido cheio: para cima 26 m de altura e 0 m de lado; para a frente 12 m de altura e 51 m. Teste por script: scratchpad `tm.py` (não salvo em Tools).

~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
