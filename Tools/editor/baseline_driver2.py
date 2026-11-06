"""Re-mede a baseline (só CsvProfile) nas câmeras fixas, com categorias RHI/DrawCall ligadas.

Versão NÃO-BLOQUEANTE: tudo agendado por ticker, sem time.sleep.
    python3 -I Tools/ue_exec.py -f Tools/editor/baseline_driver2.py
Depois (~55 s):
    python3 -I Tools/ue_exec.py -f Tools/editor/baseline_ler2.py
"""
import glob
import math
import os
import time

import unreal

SAVED = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld/Saved")
PONTOS = [
    ("P1_VistaGeral", (2600.0, -7200.0, 1400.0), (0.0, -2650.0, 100.0)),
    ("P2_Ponte", (900.0, -4300.0, 350.0), (0.0, -2650.0, 60.0)),
    ("P3_Treino", (-600.0, 5200.0, 700.0), (600.0, 1500.0, 100.0)),
]
PONTO_S = 17.0

if not hasattr(unreal, "_bl2"):
    unreal._bl2 = None


def rot_para(pos, alvo):
    dx, dy, dz = (alvo[0] - pos[0], alvo[1] - pos[1], alvo[2] - pos[2])
    return unreal.Rotator(pitch=math.degrees(math.atan2(dz, math.hypot(dx, dy))),
                          yaw=math.degrees(math.atan2(dy, dx)), roll=0.0)


def novo():
    w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ev = []
    for i, (rot, pos, alvo) in enumerate(PONTOS):
        b = i * PONTO_S
        ev += [(b + 0.3, "cam", (rot, pos, alvo)),
               (b + 3.0, "ini", rot),
               (b + 13.0, "fim", rot)]
    ev.append((len(PONTOS) * PONTO_S + 2.0, "fim_geral", None))
    ev.sort(key=lambda e: e[0])
    return {"mapa": w.get_path_name() if w else None, "t0": time.time(),
            "ev": ev, "i": 0, "f0": None, "f0t": None, "fps": {},
            "antes": set(glob.glob(os.path.join(SAVED, "Profiling", "CSV", "*.csv"))),
            "novos": [], "log": [], "done": False}


def faz(acao, arg, st, w):
    if acao == "cam":
        rot, pos, alvo = arg
        ue = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        ue.set_level_viewport_camera_info(
            unreal.Vector(pos[0], pos[1], pos[2]), rot_para(pos, alvo))
        st["log"].append("cam %s" % rot)
    elif acao == "ini":
        st["f0"] = unreal.SystemLibrary.get_frame_count()
        st["f0t"] = time.time()
    elif acao == "fim":
        df = unreal.SystemLibrary.get_frame_count() - (st["f0"] or 0)
        dt = time.time() - (st["f0t"] or time.time())
        if df > 0 and dt > 0:
            st["fps"][arg] = {"frames": df, "dur": round(dt, 2),
                              "fps": round(df / dt, 2),
                              "ms": round(1000.0 * dt / df, 2)}
        st["f0"] = None
        unreal.SystemLibrary.execute_console_command(w, "CsvProfile STOP")
    elif acao == "fim_geral":
        agora = set(glob.glob(os.path.join(SAVED, "Profiling", "CSV", "*.csv")))
        st["novos"] = sorted(os.path.basename(x) for x in (agora - st["antes"]))
        st["done"] = True
        st["log"].append("fim")


_MUNDO = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def cb(_dt):
    st = unreal._bl2
    if st is None or st["done"]:
        return False
    st["t"] = time.time() - st["t0"]
    while st["i"] < len(st["ev"]) and st["ev"][st["i"]][0] <= st["t"]:
        _t, acao, arg = st["ev"][st["i"]]
        st["i"] += 1
        try:
            if acao == "ini":
                faz("ini", arg, st, _MUNDO)
                unreal.SystemLibrary.execute_console_command(_MUNDO, "CsvProfile START")
            else:
                faz(acao, arg, st, _MUNDO)
        except Exception as e:
            st["log"].append("ERRO %s: %s" % (acao, e))
    return not st["done"]


if getattr(unreal, "_bl2", None) and not unreal._bl2["done"]:
    print("driver2 ja ativo")
else:
    unreal._bl2 = novo()
    unreal.register_ticker_callback(cb)
    print("driver2 iniciado; duracao %.0fs" % (len(PONTOS) * PONTO_S))
