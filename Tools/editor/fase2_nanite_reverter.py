"""Fase 2 — reverte o lote: desliga Nanite (com build/PostEditChange) e salva.

Uso (raiz do projeto):
    python3 -I Tools/ue_exec.py -f Tools/editor/fase2_nanite_reverter.py

Desliga via StaticMeshEditorSubsystem.set_nanite_settings(..., True), que
chama PostEditChange() -> Build() e remove a representacao Nanite.
Nao usa time.sleep.
"""
import json
import os

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
CFG = os.path.join(RAIZ, "Tools", "editor", "fase2_lote_atual.json")

sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
with open(CFG, encoding="utf-8") as f:
    cfg = json.load(f)

print("REVERTENDO lote:", cfg["lote"], "| alvos:", len(cfg["alvos"]))
salvos = 0
for p in cfg["alvos"]:
    m = unreal.EditorAssetLibrary.load_asset(p)
    if m is None:
        print("  NAO CARREGOU:", p)
        continue
    ns = m.get_editor_property("nanite_settings")
    if not ns.enabled:
        print("  ja desligado:", p.split("/")[-1])
        continue
    ns.enabled = False
    sub.set_nanite_settings(m, ns, True)   # -> PostEditChange -> Build
    m2 = unreal.EditorAssetLibrary.load_asset(p)
    tri = m2.get_num_nanite_triangles()
    ok = (not m2.get_editor_property("nanite_settings").enabled) and tri == 0
    if ok and unreal.EditorAssetLibrary.save_asset(p):
        salvos += 1
    print("  revert:", p.split("/")[-1], "| nanite=", m2.get_editor_property("nanite_settings").enabled,
          "| tri_nanite=", tri, "| tri0=", m2.get_num_triangles(0), "| lods=", m2.get_num_lods(),
          "| OK" if ok else "| FALHOU")

print("salvos:", salvos, "/", len(cfg["alvos"]))
