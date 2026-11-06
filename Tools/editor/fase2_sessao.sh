#!/bin/bash
# Uma sessão limpa da medição do Lote 1: fecha o editor, coloca as 11 malhas no estado pedido,
# abre o editor, espera assentar e mede.
# Uso: fase2_sessao.sh <rotulo> <pasta com os 11 .uasset, ou "manter">
set -u
RAIZ="$HOME/Documents/Unreal Projects/HypeWorld"
UE="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app"
INI="$HOME/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Config/MacEditor/EditorSettings.ini"
LOG="$HOME/Library/Logs/Unreal Engine/HypeWorldEditor/HypeWorld.log"
ROTULO="$1"; ESTADO="$2"
cd "$RAIZ"
SAIDA="Docs/Desempenho/Nanite_Lote1/$ROTULO"
if pgrep -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" >/dev/null; then
  python3 -I Tools/ue_exec.py "d=unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()+unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages(); print('SUJOS',[p.get_name() for p in d]); (not d) and unreal.SystemLibrary.quit_editor()" | tail -1
  for i in $(seq 1 60); do pgrep -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" >/dev/null || break; sleep 2; done
  if pgrep -f "UnrealEditor.app/Contents/MacOS/UnrealEditor" >/dev/null; then echo "EDITOR NAO FECHOU"; exit 2; fi
fi
sleep 5
if [ "$ESTADO" != "manter" ]; then
  while read -r alvo; do
    n=$(basename "$alvo"); c="Content/${alvo#/Game/}.uasset"
    [ -f "$ESTADO/$n.uasset" ] && [ -f "$c" ] || { echo "FALTA $n"; exit 3; }
    cp -p "$ESTADO/$n.uasset" "$c"
  done < <(python3 -I -c "import json;print('\n'.join(json.load(open('Tools/editor/fase2_lote_atual.json'))['alvos']))")
fi
sed -i '' 's/bThrottleCPUWhenNotForeground=True/bThrottleCPUWhenNotForeground=False/' "$INI"
echo "{\"rotulo\": \"$ROTULO\", \"rodadas\": 4, \"segundos\": 12}" > Tools/editor/fase2_medicao.json
open -a "$UE" --args "$RAIZ/HypeWorld.uproject"
for i in $(seq 1 80); do
  python3 -I Tools/ue_exec.py "print('EDITOR_PRONTO')" 2>/dev/null | grep -q EDITOR_PRONTO && break
  sleep 15
done
echo "pronto $(date +%H:%M:%S)"
sleep 90
open -a "$UE"
sleep 5
echo "shaders compilando: $(pgrep -f ShaderCompileWorker | wc -l)"
if [ "${3:-}" = "sem_medir" ]; then exit 0; fi
python3 -I Tools/ue_exec.py -f Tools/editor/fase2_medir.py | tail -1
for i in $(seq 1 60); do [ -f "$SAIDA/medicao.json" ] && break; sleep 10; done
sleep 5
[ -f "$SAIDA/medicao.json" ] || { echo "MEDICAO NAO TERMINOU"; exit 4; }
# guarda o trecho do ProfileGPU desta medição junto dos números
grep -n "" "$LOG" >/dev/null
awk '/MEDIR_PROFILE_/{p=1} p' "$LOG" > "$SAIDA/profilegpu.log"
echo "FIM $ROTULO $(date +%H:%M:%S)"
