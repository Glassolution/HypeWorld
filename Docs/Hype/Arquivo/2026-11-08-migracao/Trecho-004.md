---
status: arquivado
atualizado: 2026-11-08
---
# Ambiente — trecho preservado

> Conteúdo original preservado nesta migração; não é instrução vigente.
> Origem e ordem exatas constam em Originais.json; não ler por padrão.

<!-- HW-INICIO-ORIGINAL -->
~~~~~~~~~~markdown
### Ambiente
- **Máquina de desenvolvimento:** Mac (macOS 26.6.2). Chip **Apple M1**, **8 GB de RAM**. ~45 GB livres em disco (out/2026).
  - 8 GB é o limite mínimo para UE5 + C++: compilar com o editor aberto pode travar. Preferir compilar com o editor fechado, Lumen/Nanite desligados no editor e mapas pequenos.
- **Unreal Engine 5.8.4** (`/Users/Shared/Epic Games/UE_5.8`), com Xcode 27.1 e Metal Toolchain já configurados.
- **Plataforma final:** Windows, porque meus amigos jogam no PC. O Mac não gera build de Windows. O plano é desenvolver no Mac e gerar o build Windows em outra máquina mais tarde (ainda a definir).
- **Projeto Unreal:** `HypeWorld` (C++, template Third Person) em `/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld/` (GitHub `Glassolution/HypeWorld`, branch `main`). **Desde 25/10/2026 o repositório é a fonte oficial** de `CLAUDE.md`, `Tools/`, `Referencias/HUD`, `Referencias/UI`, `Referencias/ref_*.png` e `Fontes/`. A pasta `Desktop/Hype World` é uma CÓPIA atualizada pelo "Atualizar e Abrir" (os scripts usam caminhos absolutos dela) e guarda também o que é grande demais para o Git (Fab, Mixamo, Floresta, Referencias/Aura e Referencias/Videos). Editar sempre a versão do repositório.
- **Atualizar e Abrir (um clique):** Mac = `Desktop/Hype World/Atualizar e Abrir.command` (ou `Atualizar e Abrir Editor.command`) → `Tools/atualizar_e_abrir.sh`: git pull + lfs pull → copia Tools/Referencias/Fontes/CLAUDE.md para o Desktop → compila → roda no editor os scripts novos de `Tools/pendentes.txt` (os já rodados ficam em `Saved/scripts_rodados.txt`) → abre o jogo (ou o editor). Recusa se a Unreal estiver aberta ou houver mudança local sem commit. Windows = `Tools/atualizar_e_abrir.bat` (pull + compila + abre; NÃO roda os scripts do editor; ainda não testado).
- **Conexão com a Unreal (sem MCP, decisão do Felipe):**
  - **Código C++:** Claude escreve os arquivos `.h/.cpp` direto em `Source/` e compila pelo terminal (`Build.sh` / UnrealBuildTool). No Mac **não existe Live Coding** (verificado na 5.8.4): para ver C++ novo, o editor precisa fechar e reabrir. Salvar tudo antes de fechar.
  - **Editor aberto:** Python Remote Execution (`remote_execution.py`, oficial da engine) para criar assets, montar mapas e configurar coisas no editor em tempo real. Usar `python3 -I Tools/ue_exec.py "<código>"` ou `-f arquivo.py`. O script tem uma correção para macOS (multicast não chega em socket preso a 127.0.0.1); o editor continua só em 127.0.0.1.
- **Ferramentas conectadas via MCP:**
  - **Blender MCP:** limpeza de malhas, rigging, retarget para o esqueleto da UE5 e exportação em FBX.
  - **Higgsfield MCP:** geração de modelos 3D, animações de personagem, imagens e texturas para VFX. **Consome créditos.**


~~~~~~~~~~
<!-- HW-FIM-ORIGINAL -->

Relacionada: [[Indice]].
