"""Baseline de desempenho Fase 1 (SOMENTE LEITURA) — pontos de câmera fixos.

Para cada ponto:
  1) posiciona a câmera do viewport do editor;
  2) mede FPS por ticker (3 s);
  3) CsvProfile START -> 5 s -> STOP (FrameTime / RHI/DrawCalls / RHI/PrimitivesDrawn);
  4) DumpDetailedPrimitives (triângulos visíveis na câmera, por primitiva);
  5) take_high_res_screenshot.

Resultado em Saved/baseline_fase1.json + CSVs em Saved/Profiling/.
Rodar da raiz do projeto:
    python3 -I Tools/ue_exec.py -f Tools/editor/baseline_fase1.py
"""
import glob
import json
import os
import time

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
SAVED = os.path.join(RAIZ, "Saved")

# Ponto -> (rotulo, posicao, alvo)  [cm]
PONTOS = [
    ("P1_VistaGeral", (2600, -7200, 1400), (0, -2650, 100)),   # olha a ponte/vila
    ("P2_Ponte", (900, -4300, 350), (0, -2650, 60)),           # de frente pra ponte
    ("P3_Treino", (-600, 5200, 700), (600, 1500, 100)),        # area de treino/foliage
]


def rotacao_para(origem, alvo):
    dx = alvo[0] - origem[0]
    dy = alvo[1] - origem[1]
    dz = alvo[2] - origem[2]
    yaw = unreal.MathLibrary.radians_to_degrees(unreal.MathLibrary.atan2(dy, dx))
    horiz = (dx * dx + dy * dy) ** 0.5
    pitch = unreal.MathLibrary.radians_to_degrees(unreal.MathLibrary.atan2(dz, horiz))
    return unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw)


def medir_fps(segundos=3.0):
    """Mede frames por ticker sem bloquear o game thread."""
    st = {"t0": time.time(), "f0": unreal.SystemLibrary.get_frame_count(),
          "frames": 0, "dur": 0.0, "done": False}

    def cb(_dt):
        if time.time() - st["t0"] >= segundos:
            st["frames"] = unreal.SystemLibrary.get_frame_count() - st["f0"]
            st["dur"] = time.time() - st["t0"]
            st["done"] = True
            return False
        return True

    unreal.register_ticker_callback(cb)
    while not st["done"]:
        time.sleep(0.2)
    fps = st["frames"] / st["dur"] if st["dur"] else 0.0
    return round(fps, 2), round(1000.0 / fps, 2) if fps else None


def csv_mais_recente(pasta, padrao):
    arqs = glob.glob(os.path.join(pasta, padrao))
    if not arqs:
        return None
    return max(arqs, key=os.path.getmtime)


def ler_frame_csv(caminho):
    """Média de FrameTime, RHI/DrawCalls e RHI/PrimitivesDrawn."""
    if not caminho or not os.path.exists(caminho):
        return None
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        cab = f.readline().rstrip("\n").split(",")
        idx = {n: i for i, n in enumerate(cab)}
        col_ft = idx.get("FrameTime")
        col_dc = idx.get("RHI/DrawCalls")
        col_pr = idx.get("RHI/PrimitivesDrawn")
        soma = {"FrameTime": 0.0, "RHI/DrawCalls": 0.0, "RHI/PrimitivesDrawn": 0.0}
        n = 0
        for linha in f:
            p = linha.rstrip("\n").split(",")
            if col_ft is None or col_ft >= len(p):
                continue
            try:
                ft = float(p[col_ft])
            except ValueError:
                continue
            if ft <= 0:
                continue
            soma["FrameTime"] += ft
            if col_dc is not None and col_dc < len(p):
                try:
                    soma["RHI/DrawCalls"] += float(p[col_dc])
                except ValueError:
                    pass
            if col_pr is not None and col_pr < len(p):
                try:
                    soma["RHI/PrimitivesDrawn"] += float(p[col_pr])
                except ValueError:
                    pass
            n += 1
    if n == 0:
        return None
    return {"frames_medidos": n,
            "frame_time_ms": round(soma["FrameTime"] / n, 2),
            "draw_calls": round(soma["RHI/DrawCalls"] / n, 1),
            "primitives_drawn": round(soma["RHI/PrimitivesDrawn"] / n, 1)}


def ler_primitivas(caminho):
    """Soma de triângulos/draw calls das primitivas VISÍVEIS na câmera."""
    if not caminho or not os.path.exists(caminho):
        return None
    import csv
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        linhas = list(csv.DictReader(f))
    tri = 0
    draws = 0
    top = []
    for r in linhas:
        try:
            t = int(r.get("Triangles") or 0)
            d = int(r.get("NumDraws") or 0)
        except ValueError:
            continue
        tri += t
        draws += d
        nome = (r.get("Actor") or "").split(".")[-1][:60]
        top.append((t, nome))
    top.sort(reverse=True)
    return {"primitivas_visiveis": len(linhas),
            "triangulos_visiveis": tri,
            "draw_calls_visiveis": draws,
            "top8": [{"tri": t, "ator": n} for t, n in top[:8]]}


def main():
    ue = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    w = ue.get_editor_world()
    print("MAPA:", w.get_path_name())

    resultados = {"mapa": w.get_path_name(), "engine":
                  unreal.SystemLibrary.get_engine_version(),
                  "nanite_project": bool(unreal.SystemLibrary.get_console_variable_bool_value(
                      "r.Nanite.ProjectEnabled")),
                  "pontos": []}

    pasta_csv = os.path.join(SAVED, "Profiling", "CSV")
    pasta_prim = os.path.join(SAVED, "Profiling", "Primitives")
    os.makedirs(pasta_prim, exist_ok=True)

    # deixa as stats de unidade ligadas só para registro no log (HUD não vai pro screenshot)
    unreal.SystemLibrary.execute_console_command(w, "stat none")

    for rotulo, pos, alvo in PONTOS:
        rot = rotacao_para(pos, alvo)
        ue.set_level_viewport_camera_info(
            unreal.Vector(pos[0], pos[1], pos[2]), rot)
        time.sleep(1.5)  # deixa a câmera assentar

        fps, ms = medir_fps(3.0)

        antes_csv = set(glob.glob(os.path.join(pasta_csv, "*.csv")))
        unreal.SystemLibrary.execute_console_command(w, "CsvProfile START")
        time.sleep(5.0)
        unreal.SystemLibrary.execute_console_command(w, "CsvProfile STOP")
        time.sleep(2.0)
        novos = set(glob.glob(os.path.join(pasta_csv, "*.csv"))) - antes_csv
        csv_frame = max(novos, key=os.path.getmtime) if novos else None

        antes_p = set(glob.glob(os.path.join(pasta_prim, "*.csv")))
        unreal.SystemLibrary.execute_console_command(w, "DumpDetailedPrimitives")
        time.sleep(4.0)
        novos_p = set(glob.glob(os.path.join(pasta_prim, "*.csv"))) - antes_p
        csv_prim = max(novos_p, key=os.path.getmtime) if novos_p else None

        shot = unreal.AutomationLibrary.take_high_res_screenshot(
            1600, 900, "Fase1_Baseline_%s" % rotulo)
        time.sleep(4.0)

        r = {
            "ponto": rotulo,
            "posicao": list(pos),
            "alvo": list(alvo),
            "fps": fps,
            "frame_ms_medido": ms,
            "csv_frame": os.path.basename(csv_frame) if csv_frame else None,
            "csv_primitivas": os.path.basename(csv_prim) if csv_prim else None,
            "screenshot": "Fase1_Baseline_%s.png" % rotulo,
            "frame_csv": ler_frame_csv(csv_frame),
            "primitivas": ler_primitivas(csv_prim),
        }
        resultados["pontos"].append(r)
        print("PONTO", rotulo, "| fps", fps, "| frame_ms", ms)
        if r["frame_csv"]:
            print("   csv:", r["frame_csv"])
        if r["primitivas"]:
            p = r["primitivas"]
            print("   visiveis:", p["primitivas_visiveis"],
                  "| triangulos:", p["triangulos_visiveis"],
                  "| draws:", p["draw_calls_visiveis"])

    out = os.path.join(SAVED, "baseline_fase1.json")
    with open(out, "w", encoding="utf-8") as f:
        json.dump(resultados, f, ensure_ascii=False, indent=1)
    print("GRAVADO:", out)


main()
