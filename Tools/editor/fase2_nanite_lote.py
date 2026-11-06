"""Fase 2 — ativa Nanite nos alvos de um lote (uma lista por vez) e salva.

Uso (raiz do projeto):
    python3 -I Tools/ue_exec.py -f Tools/editor/fase2_nanite_lote.py

A lista de alvos fica em Tools/editor/fase2_lote_atual.json (gerado por
Tools/editor/fase2_lotes.py). Somente altera nanite_settings.enabled e salva
o asset. Nao mexe em colisao, materiais, LODs nem em nada de personagem.
"""
import json
import os
import shutil
import time

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
CFG = os.path.join(RAIZ, "Tools", "editor", "fase2_lote_atual.json")
BACKUP = os.path.join(RAIZ, "Saved", "Backup_Fase2")


def caminho_asset_pkg(path):
    """Converte /Game/X/Nome em caminho de arquivo .uasset."""
    assert path.startswith("/Game/"), path
    rel = path[len("/Game/"):]
    return os.path.join(RAIZ, "Content", rel + ".uasset")


def main():
    with open(CFG, encoding="utf-8") as f:
        cfg = json.load(f)
    lote = cfg["lote"]
    alvos = cfg["alvos"]
    acao = cfg.get("acao", "nanite_on")  # nanite_on | nanite_off

    print("LOTE:", lote, "| acao:", acao, "| alvos:", len(alvos))

    # 1) backup por copia (nao mexe no repo git)
    pasta_bak = os.path.join(BACKUP, lote)
    os.makedirs(pasta_bak, exist_ok=True)
    feitos = []
    for p in alvos:
        src = caminho_asset_pkg(p)
        if not os.path.exists(src):
            print("  FALTA ARQUIVO:", src)
            continue
        dst = os.path.join(pasta_bak, os.path.basename(src))
        if not os.path.exists(dst):
            shutil.copy2(src, dst)
        feitos.append(p)
    print("  backup em:", pasta_bak, "| copiados:", len(feitos))

    # 2) altera nanite
    erros = []
    ok = []
    for p in alvos:
        try:
            mesh = unreal.EditorAssetLibrary.load_asset(p)
        except Exception as e:
            erros.append((p, "load: %s" % e))
            continue
        if mesh is None:
            erros.append((p, "asset nao carregou"))
            continue
        try:
            antes = bool(mesh.get_editor_property("nanite_settings").enabled)
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = (acao == "nanite_on")
            mesh.set_editor_property("nanite_settings", ns)
            depois = bool(mesh.get_editor_property("nanite_settings").enabled)
            ok.append({"path": p, "antes": antes, "depois": depois,
                       "tri0": mesh.get_num_triangles(0),
                       "lods": mesh.get_num_lods()})
        except Exception as e:
            erros.append((p, "nanite: %s" % e))

    # 3) salva os assets alterados
    salvos = 0
    for r in ok:
        if r["antes"] != r["depois"]:
            try:
                if unreal.EditorAssetLibrary.save_asset(r["path"]):
                    salvos += 1
            except Exception as e:
                erros.append((r["path"], "save: %s" % e))
        time.sleep(0.05)

    print("  alterados:", sum(1 for r in ok if r["antes"] != r["depois"]),
          "| salvos:", salvos, "| erros:", len(erros))
    for p, e in erros:
        print("  ERRO", p, "->", e)

    # 4) confere persistencia recarregando
    conf = []
    for r in ok:
        m2 = unreal.EditorAssetLibrary.load_asset(r["path"])
        conf.append({"path": r["path"], "nanite": bool(m2.get_editor_property("nanite_settings").enabled),
                     "lods": m2.get_num_lods(),
                     "tri_nanite": m2.get_num_nanite_triangles()})

    out = os.path.join(RAIZ, "Saved", "fase2_lote_%s.json" % lote)
    with open(out, "w", encoding="utf-8") as f:
        json.dump({"lote": lote, "acao": acao, "alteracoes": ok, "conferencia": conf,
                   "erros": [list(x) for x in erros],
                   "backup": pasta_bak, "ts": time.time()}, f, ensure_ascii=False, indent=1)
    print("  GRAVADO:", out)
    for c in conf:
        print("   conf:", os.path.basename(c["path"]), "nanite=", c["nanite"],
              "lods=", c["lods"], "tri_nanite=", c["tri_nanite"])


main()
