Base aprovada para esta restauração: `f091ba9`, com a `A_Pose_Prontidao` original religada.
Restaurado: `ABP_Ninja`; `NinjaAnimInstance.cpp` recomposto da base com a reconexão autorizada; manequim visível no BP.
Conferidos sem diferenças: 214 assets de animação, incluindo pose, blend spaces, montages, esquivas e IK Rigs/Retargeters; NinjaMovementComponent e header da AnimInstance.
Removidos com backup: `NinjaBodyAnimInstance.cpp/.h` e `A_Pose_Prontidao_Base`; `ABP_Shisui` agora herda de AnimInstance, sem torção procedural.
Validação: compilação passou; trote em quatro direções a 9 m/s, corrida ninja a 14 m/s, guarda parada e manequim visível em todos os quadros.
Preservados por hash: 2216 arquivos, incluindo mapa, jutsus e VFX; nenhum commit criado.
Ressalva: a base escolhida usa máscara do tronco em C++ desde spine_01; não contém nó Layered Blend per Bone. Quatro esquivas e Tigre executados no manequim; todos tocaram suas montagens.
Vídeo: [Etapa1_Manequim.mp4](Etapa1_Manequim.mp4). Etapa 2 não iniciada; aguarda aprovação.
