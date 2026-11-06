"""Le o resultado do baseline_driver2 e grava Saved/baseline_fase1.csv_draws.json."""
import json
import os

import unreal

SAVED = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld/Saved")
PONTOS = ["P1_VistaGeral", "P2_Ponte", "P3_Treino"]
ALVOS = ("FrameTime", "RHI/DrawCalls", "RHI/PrimitivesDrawn",
         "DrawCall/Basepass", "DrawCall/ShadowDepths", "DrawCall/Prepass",
         "DrawCall/Lights")


def ler(caminho):
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        cab = f.readline().rstrip("\n").split(",")
        idx = {n: i for i, n in enumerate(cab)}
        tem = [c for c in ALVOS if c in idx]
        soma = dict.fromkeys(tem, 0.0)
        mx = dict.fromkeys(tem, 0.0)
        n = 0
        for linha in f:
            p = linha.rstrip("\n").split(",")
            try:
                ft = float(p[idx["FrameTime"]])
            except (ValueError, KeyError, IndexError):
                continue
            if ft <= 0:
                continue
            n += 1
            soma["FrameTime"] += ft
            mx["FrameTime"] = max(mx["FrameTime"], ft)
            for c in tem:
                if c == "FrameTime":
                    continue
                try:
                    v = float(p[idx[c]])
                except (ValueError, IndexError):
                    continue
                soma[c] += v
                mx[c] = max(mx[c], v)
    if not n:
        return None
    return {"frames": n,
            "frame_time_ms_media": round(soma["FrameTime"] / n, 2),
            "frame_time_ms_max": round(mx["FrameTime"], 2),
            "draw_calls_media": round(soma.get("RHI/DrawCalls", 0) / n, 1),
            "draw_calls_max": round(mx.get("RHI/DrawCalls", 0), 1),
            "primitives_drawn_media": round(soma.get("RHI/PrimitivesDrawn", 0) / n, 1),
            "basepass_media": round(soma.get("DrawCall/Basepass", 0) / n, 1),
            "shadow_media": round(soma.get("DrawCall/ShadowDepths", 0) / n, 1),
            "prepass_media": round(soma.get("DrawCall/Prepass", 0) / n, 1),
            "lights_media": round(soma.get("DrawCall/Lights", 0) / n, 1)}


st = getattr(unreal, "_bl2", None)
if st is None:
    print("driver2 nao iniciado")
    raise SystemExit(1)
print("done:", st["done"], "| log:", st["log"])

pasta = os.path.join(SAVED, "Profiling", "CSV")
saida = {"mapa": st["mapa"], "csvs": st["novos"], "pontos": [], "fps": st["fps"]}
for i, p in enumerate(PONTOS):
    d = ler(os.path.join(pasta, st["novos"][i])) if i < len(st["novos"]) else None
    saida["pontos"].append({"ponto": p, "frame_csv": d,
                            "screenshot": "Fase1_Baseline_%s.png" % p})
    print("---", p)
    print("   ticker:", st["fps"].get(p))
    print("   csv   :", d)

out = os.path.join(SAVED, "baseline_fase1_draws.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(saida, f, ensure_ascii=False, indent=1)
print("GRAVADO:", out)
