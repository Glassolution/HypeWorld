#!/bin/bash
# Atualizar e abrir o Hype World no Mac (um clique: "Atualizar e Abrir.command" na pasta Desktop/Hype World).
#   1. git pull (pega o que o Claude da nuvem mandou para o GitHub)
#   2. copia Tools/, Referencias/, Fontes/ e CLAUDE.md do repositório para Desktop/Hype World (os scripts usam esse caminho)
#   3. compila o C++ (editor fechado)
#   4. roda no editor os scripts novos listados em Tools/pendentes.txt (cada um só uma vez nesta máquina)
#   5. abre o jogo (ou o editor: ./atualizar_e_abrir.sh editor)
# Nunca fecha um editor/jogo aberto: se tiver um aberto, pede para fechar.

PROJ="/Users/luisfelipexavier/Documents/Unreal Projects/HypeWorld"
DESK="/Users/luisfelipexavier/Desktop/Hype World"
UE="/Users/Shared/Epic Games/UE_5.8"
EDITOR_APP="$UE/Engine/Binaries/Mac/UnrealEditor.app"
EDITOR_BIN="$EDITOR_APP/Contents/MacOS/UnrealEditor"
UPROJECT="$PROJ/HypeWorld.uproject"
DONE_LIST="$PROJ/Saved/scripts_rodados.txt"
EDITOR_INI="$HOME/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Config/MacEditor/EditorSettings.ini"
MODE="${1:-jogo}"

passo() { echo; echo "=== $1 ==="; }
falhou() { echo; echo "XXX $1"; echo "(nada foi aberto; leia a mensagem acima)"; exit 1; }
esperar() { python3 -c "import time; time.sleep($1)"; }
editor_aberto() { pgrep -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" >/dev/null; }

if editor_aberto; then
    falhou "A Unreal (editor ou jogo) está aberta. Salve, feche e rode de novo."
fi

passo "1/5 Puxando do GitHub"
cd "$PROJ" || falhou "Projeto não encontrado em $PROJ"
if [ -n "$(git status --porcelain)" ]; then
    echo "Você tem mudanças locais ainda sem commit:"
    git status --short
    falhou "Faça commit (ou peça ao Claude) antes de puxar, para nada se perder."
fi
git pull --ff-only origin main || falhou "Não deu para puxar (sem internet ou histórico diferente)."
git lfs pull || echo "(aviso: git lfs pull falhou; assets grandes podem estar desatualizados)"
git log --oneline -1

passo "2/5 Copiando ferramentas para Desktop/Hype World"
mkdir -p "$DESK"
rsync -a --exclude "__pycache__" "$PROJ/Tools/" "$DESK/Tools/"
rsync -a "$PROJ/Referencias/" "$DESK/Referencias/"
rsync -a "$PROJ/Fontes/" "$DESK/Fontes/"
cp "$PROJ/CLAUDE.md" "$DESK/CLAUDE.md"
echo "ok"

passo "3/5 Compilando o C++"
"$UE/Engine/Build/BatchFiles/Mac/Build.sh" HypeWorldEditor Mac Development -project="$UPROJECT" > "$PROJ/Saved/ultima_compilacao.log" 2>&1
if ! grep -q "Result: Succeeded" "$PROJ/Saved/ultima_compilacao.log"; then
    grep -iE "error" "$PROJ/Saved/ultima_compilacao.log" | head -20
    falhou "A compilação falhou. Mande esses erros para o Claude (log completo: Saved/ultima_compilacao.log)."
fi
echo "compilou"

# O editor regrava este ajuste ao fechar; ligado, os testes rodam a 3 FPS com o editor em segundo plano
sed -i '' 's/bThrottleCPUWhenNotForeground=True/bThrottleCPUWhenNotForeground=False/' "$EDITOR_INI" 2>/dev/null

passo "4/5 Scripts novos do editor"
touch "$DONE_LIST"
PENDING=()
while IFS= read -r line; do
    line="${line%%#*}"; line="$(echo "$line" | xargs)"
    [ -z "$line" ] && continue
    grep -qxF "$line" "$DONE_LIST" || PENDING+=("$line")
done < "$PROJ/Tools/pendentes.txt"

if [ ${#PENDING[@]} -eq 0 ]; then
    echo "nenhum"
else
    echo "Abrindo o editor para rodar: ${PENDING[*]}"
    open -a "$EDITOR_APP" --args "$UPROJECT"
    ready=0
    for i in $(seq 1 120); do
        if python3 -I "$DESK/Tools/ue_exec.py" "print('PRONTO')" 2>/dev/null | grep -q PRONTO; then ready=1; break; fi
        esperar 3
    done
    [ $ready -eq 1 ] || falhou "O editor não respondeu em 6 minutos."
    for s in "${PENDING[@]}"; do
        echo "--- $s"
        out=$(python3 -I "$DESK/Tools/ue_exec.py" -f "$DESK/$s" 2>&1)
        echo "$out" | tail -15
        if echo "$out" | grep -qE "Traceback|Error:"; then
            echo "!!! $s deu erro (não marquei como feito). Mande a saída acima para o Claude."
        else
            echo "$s" >> "$DONE_LIST"
        fi
    done
    python3 -I "$DESK/Tools/ue_exec.py" "import unreal; unreal.SystemLibrary.quit_editor()" >/dev/null 2>&1
    for i in $(seq 1 60); do editor_aberto || break; esperar 2; done
    sed -i '' 's/bThrottleCPUWhenNotForeground=True/bThrottleCPUWhenNotForeground=False/' "$EDITOR_INI" 2>/dev/null
fi

passo "5/5 Abrindo"
if [ "$MODE" = "editor" ]; then
    open -a "$EDITOR_APP" --args "$UPROJECT"
    echo "Editor abrindo."
else
    nohup "$EDITOR_BIN" "$UPROJECT" -game -windowed -ResX=1600 -ResY=900 > /dev/null 2>&1 &
    echo "Jogo abrindo (janela 1600x900). Para fechar: Cmd+Q."
fi
