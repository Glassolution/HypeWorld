---
status: arquivado
atualizado: 2026-11-08
---
# 2. Pastas e ferramentas — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
## 2. Pastas e ferramentas
| O quê | Onde |
|---|---|
| Projeto Unreal | `/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld/` (`HypeWorld.uproject`, git com LFS, remoto github.com/Glassolution/HypeWorld) |
| CLAUDE.md, scripts, referências | `/Users/luisfelipexavier/Desktop/Hype World/` |
| Scripts do editor (Python) | `Desktop/Hype World/Tools/editor/*.py` |
| Ponte com o editor aberto | `python3 -I Tools/ue_exec.py "<código>"` ou `-f arquivo.py` (rodar a partir de `Desktop/Hype World`) |
| Vídeo de referência original | `~/Downloads/ScreenRecording_10-23-2026 22-50-53_1.mov` (1920x886, 60 fps, 12,9 s) |
| Trechos recortados do vídeo | `Desktop/Hype World/Referencias/Videos/ref_corrida.mp4`, `ref_dash_estrela_1.mp4`, `ref_dash_estrela_2.mp4`, `ref_pulo.mp4` |
| FBX que vierem do DeepMotion | `Desktop/Hype World/Mixamo/Captura/` (criar se não existir) |
| ffmpeg (analisar vídeo quadro a quadro) | `/opt/homebrew/bin/ffmpeg` (sem filtro drawtext; montar folhas com `tile`) |

- C++ novo: fechar o editor salvando tudo → `"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" HypeWorldEditor Mac Development -Project="<uproject>" -WaitMutex` → reabrir com `open -a "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app" --args "<uproject>"`. Antes de abrir, trocar `bThrottleCPUWhenNotForeground=True` por `False` em `~/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Config/MacEditor/EditorSettings.ini` e, depois de abrir, desligar em memória (`unreal.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings')` → `bThrottleCPUWhenNotForeground` False). Senão o jogo roda a 3 FPS.
- Python: `unreal.Rotator(a,b,c)` é (roll, pitch, yaw) — usar sempre `unreal.Rotator(pitch=, yaw=, roll=)`.
- Mac com 8 GB: evitar 2 janelas de jogo abertas.


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
