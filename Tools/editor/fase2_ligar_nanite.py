"""Fase 2 — liga o Nanite nos alvos do lote, constrói e salva (uma vez, sem medir)."""
import json
import os

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
ALVOS = json.load(open(os.path.join(RAIZ, "Tools", "editor", "fase2_lote_atual.json"), encoding="utf-8"))["alvos"]
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for p in ALVOS:
    m = unreal.EditorAssetLibrary.load_asset(p)
    ns = m.get_editor_property("nanite_settings")
    ns.enabled = True
    sub.set_nanite_settings(m, ns, True)
    m = unreal.EditorAssetLibrary.load_asset(p)
    salvo = unreal.EditorAssetLibrary.save_asset(p, only_if_is_dirty=False)
    print("LIGADO", p.split("/")[-1], "tri_nanite=", m.get_num_nanite_triangles(), "lods=", m.get_num_lods(), "salvo=", salvo)
