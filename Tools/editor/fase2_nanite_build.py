"""Fase 2 — constroi a representacao Nanite (build) dos alvos e salva.

O set_editor_property nao dispara o build; quem dispara e
StaticMeshEditorSubsystem.set_nanite_settings(..., bApplyChanges=True),
que chama PostEditChange() -> UStaticMesh::Build().

Uso (raiz do projeto):
    python3 -I Tools/ue_exec.py -f Tools/editor/fase2_nanite_build.py

Processa so os alvos com nanite ligado e tri_nanite==0, respeitando
FASE2_MAX (padrao 6) para manter lotes pequenos. Nao usa time.sleep.
"""
import json
import os

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
CFG = os.path.join(RAIZ, "Tools", "editor", "fase2_lote_atual.json")
MAX = int(os.environ.get("FASE2_MAX", "6"))

sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
with open(CFG, encoding="utf-8") as f:
    cfg = json.load(f)

pendentes = []
for p in cfg["alvos"]:
    m = unreal.EditorAssetLibrary.load_asset(p)
    if m is None:
        print("NAO CARREGOU:", p)
        continue
    ns = m.get_editor_property("nanite_settings")
    if ns.enabled and m.get_num_nanite_triangles() == 0:
        pendentes.append(p)

print("pendentes de build:", len(pendentes), "| max do lote:", MAX)
lote = pendentes[:MAX]
salvos = 0
for p in lote:
    m = unreal.EditorAssetLibrary.load_asset(p)
    ns = m.get_editor_property("nanite_settings")
    sub.set_nanite_settings(m, ns, True)   # -> PostEditChange -> Build
    m2 = unreal.EditorAssetLibrary.load_asset(p)
    tri = m2.get_num_nanite_triangles()
    if tri > 0 and unreal.EditorAssetLibrary.save_asset(p):
        salvos += 1
    print("  build:", p.split("/")[-1], "| tri_nanite=", tri, "| lods=", m2.get_num_lods())

print("salvos:", salvos, "| restam pendentes:", len(pendentes) - len(lote))

# passe final: salva qualquer alvo sujo (ex.: build feito em execucao anterior)
extra = 0
for p in cfg["alvos"]:
    try:
        if unreal.EditorAssetLibrary.save_asset(p, only_if_is_dirty=True):
            extra += 1
    except Exception as e:
        print("  ERRO save", p, "->", e)
print("salvos sujos:", extra)
