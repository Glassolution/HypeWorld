"""Junta as medições do Lote 1 numa tabela (roda fora do editor): python3 -I Tools/editor/fase2_comparar.py A1 B1 ..."""
import json
import os
import re
import statistics
import sys

BASE = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld/Docs/Desempenho/Nanite_Lote1")
PONTOS = ("P1_VistaGeral", "P2_Ponte", "P3_Treino")
PASSES = ("BasePassParallel", "DepthPassParallel", "VelocityParallel", "ShadowDepths", "Translucency")


def perfis(rotulo):
    """Tempo por passe (inclusivo, ms), por ponto, a partir do ProfileGPU da sessão."""
    caminho = os.path.join(BASE, rotulo, "profilegpu.log")
    res = {p: {} for p in PONTOS}
    if not os.path.exists(caminho):
        return res, 0
    ponto = None
    nanite = 0
    for linha in open(caminho, encoding="utf-8", errors="replace"):
        m = re.search(r"MEDIR_PROFILE_.*?_(P\d_\w+)", linha)
        if m:
            ponto = m.group(1)
            continue
        c = linha.split("┃")
        if len(c) < 4 or ponto is None:
            continue
        inc = re.search(r"([\d.]+) ms", c[2])
        nome = c[3].strip()
        if not inc:
            continue
        if "nanite" in nome.lower():
            nanite += 1
        if nome == "<root>":
            res[ponto].setdefault("Total", []).append(float(inc.group(1)))
        elif nome.startswith("TSR "):
            res[ponto].setdefault("TSR", []).append(float(inc.group(1)))
        else:
            for p in PASSES:
                if nome == p or (nome.split(" ")[0] == p):
                    res[ponto].setdefault(p, []).append(float(inc.group(1)))
                    break
    return res, nanite


for rotulo in sys.argv[1:]:
    d = json.load(open(os.path.join(BASE, rotulo, "medicao.json"), encoding="utf-8"))
    print("==", rotulo, "| nanite ligado:", sum(m["nanite"] for m in d["malhas"]), "/", len(d["malhas"]),
          "| carga:", [round(x, 1) for x in d["sistema_inicio"]["carga"]], "->", [round(x, 1) for x in d["sistema_fim"]["carga"]],
          "| erros:", d["erros"])
    pf, n = perfis(rotulo)
    print("   linhas com 'nanite' no ProfileGPU:", n)
    for p in PONTOS:
        rs = [r["valores"] for r in d["rodadas"] if r["ponto"] == p and r["rodada"] > 0 and r.get("valores")]
        med = lambda c: round(statistics.median(x[c]["mediana"] for x in rs), 2) if rs else None
        pr = d["primitivas"].get(p, {})
        ps = {k: round(max(v) if k == "TSR" else statistics.median(v), 2) for k, v in pf.get(p, {}).items()}
        print("  ", p, "quadro", med("FrameTime"), [x["FrameTime"]["mediana"] for x in rs], "gpu", med("GPUTime"), "game", med("GameThreadTime"),
              "tri", pr.get("triangulos"), "draws", pr.get("draws"), "| passes", ps)
