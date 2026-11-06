"""Lê o estado do driver da baseline e grava Saved/baseline_fase1.json."""
import csv
import glob
import json
import os

import unreal

SAVED = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld/Saved")


def ler_frame(caminho):
    if not caminho or not os.path.exists(caminho):
        return None
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        cab = f.readline().rstrip("\n").split(",")
        idx = {n: i for i, n in enumerate(cab)}
        alvo = [c for c in ("FrameTime", "RHI/DrawCalls", "RHI/PrimitivesDrawn")
                if c in idx]
        soma = dict.fromkeys(alvo, 0.0)
        n = 0
        for linha in f:
            p = linha.rstrip("\n").split(",")
            try:
                ft = float(p[idx["FrameTime"]])
            except (ValueError, KeyError, IndexError):
                continue
            if ft <= 0:
                continue
            for c in alvo:
                try:
                    soma[c] += float(p[idx[c]])
                except (ValueError, IndexError):
                    pass
            n += 1
    if not n:
        return None
    return {"frames_csv": n,
            "frame_time_ms": round(soma.get("FrameTime", 0.0) / n, 2),
            "draw_calls": round(soma.get("RHI/DrawCalls", 0.0) / n, 1),
            "primitives_drawn": round(soma.get("RHI/PrimitivesDrawn", 0.0) / n, 1)}


def ler_prim(caminho):
    if not caminho or not os.path.exists(caminho):
        return None
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        linhas = list(csv.DictReader(f))
    tri = draws = 0
    top = []
    for r in linhas:
        try:
            t = int(r.get("Triangles") or 0)
            d = int(r.get("NumDraws") or 0)
        except ValueError:
            t, d = 0, 0
        tri += t
        draws += d
        nome = (r.get("Actor") or "").split(".")[-1][:70]
        top.append((t, nome))
    top.sort(reverse=True)
    return {"primitivas_visiveis": len(linhas),
            "triangulos_visiveis": tri,
            "draw_calls_visiveis": draws,
            "top8": [{"tri": t, "ator": n} for t, n in top[:8] if t]}


st = getattr(unreal, "_bl", None)
if st is None:
    print("driver nao foi iniciado")
    raise SystemExit(1)

print("done:", st["done"], "| log:", len(st["log"]), "eventos")
res = {
    "mapa": st["mapa"],
    "engine": unreal.SystemLibrary.get_engine_version(),
    "nanite_project": bool(unreal.SystemLibrary.get_console_variable_bool_value(
        "r.Nanite.ProjectEnabled")),
    "nanite_foliage": bool(unreal.SystemLibrary.get_console_variable_bool_value(
        "r.Nanite.Foliage")),
    "pontos": [],
    "log": st["log"],
    "arquivos": {"csv": sorted(st["arquivos"].get("csv", [])),
                 "primitivas": sorted(st["arquivos"].get("primitivas", []))},
}

pasta_csv = os.path.join(SAVED, "Profiling", "CSV")
pasta_prim = os.path.join(SAVED, "Profiling", "Primitives")
csvs = st["arquivos"].get("csv", [])
prims = st["arquivos"].get("primitivas", [])

pontos = ["P1_VistaGeral", "P2_Ponte", "P3_Treino"]
for i, p in enumerate(pontos):
    fc = ler_frame(os.path.join(pasta_csv, csvs[i])) if i < len(csvs) else None
    pr = ler_prim(os.path.join(pasta_prim, prims[i])) if i < len(prims) else None
    res["pontos"].append({
        "ponto": p,
        "fps_ticker": st["fps"].get(p),
        "frame_csv": fc,
        "primitivas": pr,
        "screenshot": "Fase1_Baseline_%s.png" % p,
    })

out = os.path.join(SAVED, "baseline_fase1.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(res, f, ensure_ascii=False, indent=1)

print("GRAVADO:", out)
for p in res["pontos"]:
    print("---", p["ponto"])
    print("   ticker:", p["fps_ticker"])
    print("   csv   :", p["frame_csv"])
    print("   prims :", {k: v for k, v in (p["primitivas"] or {}).items() if k != "top8"})
    if p["primitivas"]:
        for t in p["primitivas"]["top8"]:
            print("      %6d tri  %s" % (t["tri"], t["ator"]))
