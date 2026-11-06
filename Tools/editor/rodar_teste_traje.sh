#!/bin/bash
# Roda Tools/editor/teste_traje.py no Play e copia as fotos. Editor aberto, sem Play, Unreal em primeiro plano ou com o throttle desligado.
# Uso: Tools/editor/rodar_teste_traje.sh <Personagem> [modo: vazio ou congelado]. Fotos em Saved/TesteTraje/<Personagem>/
R="$(cd "$(dirname "$0")/../.." && pwd)"
D="$R/Saved/Screenshots/MacEditor"
SAIDA="$R/Saved/TesteTraje/$1"
mkdir -p "$SAIDA"
cd "$R"
python3 -I Tools/ue_exec.py "import unreal
unreal._teste_traje_malha='/Game/HypeWorld/Personagens/Indra/Modular/Trajes/$1/SK_Outfit_$1'
unreal._teste_traje_modo='$2'
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()" >/dev/null 2>&1
python3 -c "import time;time.sleep(25)"; touch "$SAIDA/.marca"; python3 -c "import time;time.sleep(1)"
python3 -I Tools/ue_exec.py -f Tools/editor/teste_traje.py 2>&1 | grep -iE "error|Traceback" | cut -c1-200
for i in $(seq 1 40); do
  r=$(python3 -I Tools/ue_exec.py 'import unreal;print("FIM" if any(("fim" in l) or ("ERRO" in l) for l in unreal._teste_traje["log"]) else "...")' 2>/dev/null | tail -1)
  [ "$r" = "FIM" ] && break
  python3 -c "import time;time.sleep(3)"
done
python3 -I Tools/ue_exec.py 'import unreal;print("\n".join(unreal._teste_traje["log"]))' 2>&1 | tail -14 | cut -c1-200
python3 -c "import time;time.sleep(4)"
i=0; find "$D" -name "*.png" -newer "$SAIDA/.marca" | sort | while read -r f; do i=$((i+1)); cp "$f" "$SAIDA/$i.png"; done
ls "$SAIDA" | wc -l
python3 -I Tools/ue_exec.py "import unreal
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()" >/dev/null 2>&1
