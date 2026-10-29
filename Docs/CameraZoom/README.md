# Câmera e zoom

O mapeamento já estava correto: `/Game/Input/IMC_Default` usa `MouseWheelAxis → /Game/Ninja/Core/Input/IA_Zoom` (Axis1D). O scroll não estava duplicado no Look. A função `Zoom` alterava somente o alvo da distância.

O enquadramento antigo usava SocketOffset Z=185, TargetOffset Z=0 e uma inclinação de -19° na câmera relativa ao Spring Arm. Essa diferença entre o eixo do braço e o eixo óptico mudava o enquadramento vertical ao aproximar/afastar. A inclinação agora pertence ao controle do braço; a câmera relativa tem rotação zero. A inicialização também espera a rotação de spawn do controlador antes de aplicar o pitch inicial.

Os parâmetros estão no Character, em **Class Defaults > Camera**:

| Parâmetro | Padrão |
|---|---|
| Camera Zoom Default / Min / Max | 450 / 200 / 800 cm |
| Camera Zoom Step / Speed | 60 / 8 |
| Camera Target Offset | (0, 0, 60) |
| Camera Socket Offset | (0, 0, 40) |
| Camera Initial Pitch / Min / Max | -15° / -60° / +30° |
| Camera FOV | 80° |
| Camera Use Pawn Control Rotation | ativo |
| Camera Lag Enabled / Speed | ativo / 12 |
| Camera Rotation Lag Enabled | desligado |
| Camera Collision Test / Probe Size | ativo / 12 cm |

O scroll positivo aproxima; negativo afasta. `Zoom → DoCameraZoom` altera apenas o alvo de distância, e `UpdateCameraZoom` interpola `TargetArmLength` com `FInterpTo` usando `DeltaSeconds`. Pivô, offsets e rotação não são alterados durante o zoom. A posição física da câmera acompanha o braço e a correção de colisão, conforme o comportamento normal do Spring Arm.

`validation.json` registra o teste via Enhanced Input: padrão, passo de 60 cm, saturação no mínimo e no máximo, monotonicidade e invariância de pitch, offsets e posição do personagem. As três capturas estão na mesma posição, orientação e FOV.

Alterações restritas à câmera em `HypeWorldCharacter.h/.cpp` e aos padrões da câmera do `BP_ThirdPersonCharacter`. Nenhum Mapping Context, movimento, combate, animação, material ou mapa foi editado.
