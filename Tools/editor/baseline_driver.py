"""Baseline Fase 1 — versão NÃO-BLOQUEANTE.

IMPORTANTE: nenhum time.sleep aqui. Esperas longas travariam o game thread
(o ticker nunca dispararia). Tudo é agendado numa timeline por ticker.

Uso (da raiz do projeto):
    python3 -I Tools/ue_exec.py -f Tools/editor/baseline_driver.py
Depois, após ~60 s:
    python3 -I Tools/ue_exec.py -f Tools/editor/baseline_ler.py
"""
import glob
import math
import os
import time

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
SAVED = os.path.join(RAIZ, "Saved")
PONTOS = [
    ("P1_VistaGeral", (2600.0, -7200.0, 1400.0), (0.0, -2650.0, 100.0)),
    ("P2_Ponte", (900.0, -4300.0, 350.0), (0.0, -2650.0, 60.0)),
    ("P3_Treino", (-600.0, 5200.0, 700.0), (600.0, 1500.0, 100.0)),
]
DURACAO_PONTO = 16.0  # segundos por ponto na timeline

# estado persistido entre chamadas do remote execution
if not hasattr(unreal, "_bl"):
    unreal._bl = None


def rot_para(pos, alvo):
    dx, dy, dz = (alvo[0] - pos[0], alvo[1] - pos[1], alvo[2] - pos[2])
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, math.hypot(dx, dy)))
    return unreal.Rotator(pitch=pitch, yaw=yaw, roll=0.0)


def novo_estado():
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    eventos = []
    for i, (rot, pos, alvo) in enumerate(PONTOS):
        b = i * DURACAO_PONTO
        eventos.append((b + 0.5, "camera", (rot, pos, alvo)))
        eventos.append((b + 2.0, "fps_ini", rot))
        eventos.append((b + 5.0, "fps_fim", rot))
        eventos.append((b + 5.2, "csv_start", rot))
        eventos.append((b + 10.2, "csv_stop", rot))
        eventos.append((b + 11.0, "primitivas", rot))
        eventos.append((b + 11.8, "screenshot", rot))
    eventos.append((len(PONTOS) * DURACAO_PONTO + 2.0, "fim", None))
    eventos.sort(key=lambda e: e[0])
    return {
        "mapa": w.get_path_name() if w else None,
        "t0": time.time(),
        "eventos": eventos,
        "idx": 0,
        "fps": {},
        "f0": None,
        "f0t": None,
        "csv_antes": set(glob.glob(os.path.join(SAVED, "Profiling", "CSV", "*.csv"))),
        "prim_antes": set(glob.glob(os.path.join(SAVED, "Profiling", "Primitives", "*.csv"))),
        "arquivos": {},
        "log": [],
        "done": False,
        "handle": None,
    }


def executar(acao, arg, st, w):
    if acao == "camera":
        rot, pos, alvo = arg
        ue = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        ue.set_level_viewport_camera_info(
            unreal.Vector(pos[0], pos[1], pos[2]), rot_para(pos, alvo))
        st["log"].append("camera %s" % rot)
    elif acao == "fps_ini":
        st["f0"] = unreal.SystemLibrary.get_frame_count()
        st["f0t"] = time.time()
    elif acao == "fps_fim":
        if st["f0"]:
            df = unreal.SystemLibrary.get_frame_count() - st["f0"]
            dt = time.time() - st["f0t"]
            if dt > 0 and df > 0:
                st["fps"][arg] = {"frames": df, "dur": round(dt, 2),
                                  "fps": round(df / dt, 2),
                                  "ms": round(1000.0 * dt / df, 2)}
        st["f0"] = None
    elif acao == "csv_start":
        unreal.SystemLibrary.execute_console_command(w, "CsvProfile START")
    elif acao == "csv_stop":
        unreal.SystemLibrary.execute_console_command(w, "CsvProfile STOP")
    elif acao == "primitivas":
        unreal.SystemLibrary.execute_console_command(w, "DumpDetailedPrimitives")
    elif acao == "screenshot":
        unreal.AutomationLibrary.take_high_res_screenshot(
            1600, 900, "Fase1_Baseline_%s" % arg)
    elif acao == "fim":
        novos = set(glob.glob(os.path.join(SAVED, "Profiling", "CSV", "*.csv"))) - st["csv_antes"]
        novosp = set(glob.glob(os.path.join(SAVED, "Profiling", "Primitives", "*.csv"))) - st["prim_antes"]
        st["arquivos"] = {
            "csv": sorted(os.path.basename(x) for x in novos),
            "primitivas": sorted(os.path.basename(x) for x in novosp),
        }
        st["done"] = True
        st["log"].append("fim")


def cb(dt):
    st = unreal._bl
    if st is None or st["done"]:
        return False
    st["t"] = time.time() - st["t0"]
    while st["idx"] < len(st["eventos"]) and st["eventos"][st["idx"]][0] <= st["t"]:
        _t, acao, arg = st["eventos"][st["idx"]]
        st["idx"] += 1
        try:
            executar(acao, arg, st, _mundo)
        except Exception as e:  # registro e sigo, para não perder o restante
            st["log"].append("ERRO %s: %s" % (acao, e))
    if st["done"]:
        return False
    return True


_w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
_mundo = _w
if getattr(unreal, "_bl", None) and not unreal._bl["done"]:
    print("driver ja rodando; ignorando nova execucao")
else:
    unreal._bl = novo_estado()
    unreal._bl["handle"] = unreal.register_ticker_callback(cb)
    print("driver iniciado; duracao total %.0fs" % (len(PONTOS) * DURACAO_PONTO))
