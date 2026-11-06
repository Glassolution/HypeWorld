"""Fase 2 — medição limpa (NÃO-BLOQUEANTE) nas 3 câmeras da baseline.

Configuração em Tools/editor/fase2_medicao.json: {"rotulo": "A1", "rodadas": 4, "segundos": 12}
A primeira rodada é só aquecimento e fica de fora das contas.
Saída em Docs/Desempenho/Nanite_Lote1/<rotulo>/: medicao.json e fotos em Saved/Screenshots/MacEditor.

Uso (raiz do projeto, editor aberto, sem shader compilando):
    python3 -I Tools/ue_exec.py -f Tools/editor/fase2_medir.py
Termina quando medicao.json aparecer.
"""
import csv
import glob
import json
import math
import os
import statistics
import subprocess
import time

import unreal

RAIZ = os.path.expanduser("~/Documents/Unreal Projects/HypeWorld")
SAVED = os.path.join(RAIZ, "Saved")
with open(os.path.join(RAIZ, "Tools", "editor", "fase2_medicao.json"), encoding="utf-8") as _f:
    CFG = json.load(_f)
ROTULO = CFG["rotulo"]
RODADAS = int(CFG.get("rodadas", 4))
SEGUNDOS = float(CFG.get("segundos", 12))
SAIDA = os.path.join(RAIZ, "Docs", "Desempenho", "Nanite_Lote1", ROTULO)
ALVOS = json.load(open(os.path.join(RAIZ, "Tools", "editor", "fase2_lote_atual.json"), encoding="utf-8"))["alvos"]
PONTOS = [
    ("P1_VistaGeral", (2600.0, -7200.0, 1400.0), (0.0, -2650.0, 100.0)),
    ("P2_Ponte", (900.0, -4300.0, 350.0), (0.0, -2650.0, 60.0)),
    ("P3_Treino", (-600.0, 5200.0, 700.0), (600.0, 1500.0, 100.0)),
]
COLUNAS = ("FrameTime", "GPUTime", "GameThreadTime", "RenderThreadTime", "RHIThreadTime")
PASTA_CSV = os.path.join(SAVED, "Profiling", "CSV")
PASTA_PRIM = os.path.join(SAVED, "Profiling", "Primitives")
_MUNDO = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def cmd(texto):
    unreal.SystemLibrary.execute_console_command(_MUNDO, texto)


def rot_para(pos, alvo):
    dx, dy, dz = (alvo[0] - pos[0], alvo[1] - pos[1], alvo[2] - pos[2])
    return unreal.Rotator(pitch=math.degrees(math.atan2(dz, math.hypot(dx, dy))),
                          yaw=math.degrees(math.atan2(dy, dx)), roll=0.0)


def sistema():
    def saida(*c):
        try:
            return subprocess.run(c, capture_output=True, text=True, timeout=10).stdout.strip()
        except Exception as e:
            return str(e)
    return {"hora": time.strftime("%H:%M:%S"), "carga": os.getloadavg(),
            "swap": saida("/usr/sbin/sysctl", "-n", "vm.swapusage"),
            "top": saida("/bin/sh", "-c", "/bin/ps -axo pcpu,rss,comm -r | /usr/bin/head -6 | /usr/bin/cut -c1-70")}


def malhas():
    linhas = []
    for p in ALVOS:
        m = unreal.EditorAssetLibrary.load_asset(p)
        linhas.append({"malha": p.split("/")[-1],
                       "nanite": bool(m.get_editor_property("nanite_settings").enabled),
                       "tri_nanite": m.get_num_nanite_triangles(),
                       "lods": m.get_num_lods(),
                       "tri_por_lod": [m.get_num_triangles(i) for i in range(m.get_num_lods())]})
    return linhas


def ler_csv(caminho):
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        cab = f.readline().rstrip("\n").split(",")
        idx = {n: i for i, n in enumerate(cab)}
        valores = {c: [] for c in COLUNAS if c in idx}
        for linha in f:
            p = linha.rstrip("\n").split(",")
            for c in valores:
                try:
                    valores[c].append(float(p[idx[c]]))
                except (ValueError, IndexError):
                    pass
    r = {}
    for c, v in valores.items():
        v = [x for x in v[5:] if x > 0]
        if v:
            v.sort()
            r[c] = {"mediana": round(statistics.median(v), 2),
                    "p95": round(v[int(len(v) * 0.95)], 2), "quadros": len(v)}
    return r


def ler_prim(caminho):
    with open(caminho, "r", encoding="utf-8", errors="replace") as f:
        linhas = list(csv.DictReader(f))
    tri = draws = 0
    for r in linhas:
        try:
            tri += int(r.get("Triangles") or 0)
            draws += int(r.get("NumDraws") or 0)
        except ValueError:
            pass
    return {"primitivas": len(linhas), "triangulos": tri, "draws": draws}


def novo():
    ev = []
    t = 3.0
    for rodada in range(RODADAS):
        for nome, pos, alvo in PONTOS:
            ev.append((t, "cam", (pos, alvo)))
            ev.append((t + 6.0, "ini", (rodada, nome)))
            ev.append((t + 6.0 + SEGUNDOS, "fim", (rodada, nome)))
            t += 6.0 + SEGUNDOS + 2.0
            if rodada == RODADAS - 1:
                ev.append((t, "prim", nome))
                for k in (4.0, 7.0, 10.0):
                    ev.append((t + k, "gpu", nome))
                ev.append((t + 13.0, "foto", nome))
                t += 19.0
    ev.append((t, "fechar", None))
    return {"t0": time.time(), "ev": ev, "i": 0, "res": [], "prim": {}, "atual": None,
            "antes": set(), "done": False, "inicio": sistema(), "log": []}


def faz(acao, arg, st):
    if acao == "cam":
        pos, alvo = arg
        unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
            unreal.Vector(*pos), rot_para(pos, alvo))
    elif acao == "ini":
        st["antes"] = set(glob.glob(os.path.join(PASTA_CSV, "*.csv")))
        st["atual"] = (arg, unreal.SystemLibrary.get_frame_count(), time.time())
        cmd("CsvProfile START")
    elif acao == "fim":
        (rodada, nome), f0, t0 = st["atual"]
        df = unreal.SystemLibrary.get_frame_count() - f0
        dt = time.time() - t0
        cmd("CsvProfile STOP")
        st["res"].append({"rodada": rodada, "ponto": nome, "ms_ticker": round(1000.0 * dt / max(df, 1), 2),
                          "antes": st["antes"]})
    elif acao == "prim":
        st["prim_antes"] = set(glob.glob(os.path.join(PASTA_PRIM, "*.csv")))
        cmd("DumpDetailedPrimitives")
    elif acao == "gpu":
        novos = set(glob.glob(os.path.join(PASTA_PRIM, "*.csv"))) - st["prim_antes"]
        if novos and arg not in st["prim"]:
            st["prim"][arg] = ler_prim(max(novos, key=os.path.getmtime))
        unreal.log("MEDIR_PROFILE_%s_%s" % (ROTULO, arg))
        cmd("ProfileGPU")
    elif acao == "foto":
        unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, "Fase2Limpa_%s_%s" % (ROTULO, arg))
    elif acao == "fechar":
        fechar(st)


def fechar(st):
    # cada CSV é o mais antigo que apareceu depois do início da sua medição
    usados = set()
    saida = []
    for r in st["res"]:
        novos = sorted(set(glob.glob(os.path.join(PASTA_CSV, "*.csv"))) - r.pop("antes") - usados,
                       key=os.path.getmtime)
        if novos:
            usados.add(novos[0])
            r["csv"] = os.path.basename(novos[0])
            r["valores"] = ler_csv(novos[0])
        saida.append(r)
    os.makedirs(SAIDA, exist_ok=True)
    with open(os.path.join(SAIDA, "medicao.json"), "w", encoding="utf-8") as f:
        json.dump({"rotulo": ROTULO, "mapa": _MUNDO.get_path_name(), "rodadas": saida, "primitivas": st["prim"],
                   "malhas": malhas(), "sistema_inicio": st["inicio"], "sistema_fim": sistema(),
                   "erros": st["log"]}, f, ensure_ascii=False, indent=1)
    st["done"] = True
    unreal.log("MEDIR_FIM_%s" % ROTULO)


def cb(_dt):
    st = unreal._medir
    if st["done"]:
        return False
    t = time.time() - st["t0"]
    while not st["done"] and st["i"] < len(st["ev"]) and st["ev"][st["i"]][0] <= t:
        _t, acao, arg = st["ev"][st["i"]]
        st["i"] += 1
        try:
            faz(acao, arg, st)
        except Exception as e:
            st["log"].append("ERRO %s: %s" % (acao, e))
            unreal.log_error("MEDIR_ERRO %s: %s" % (acao, e))
    return not st["done"]


if getattr(unreal, "_medir", None) and not unreal._medir["done"]:
    print("medicao ja rodando")
else:
    cmd("stat none")
    cmd("r.ProfileGPU.ShowUI 0")
    unreal._medir = novo()
    unreal.register_ticker_callback(cb)
    print("medicao iniciada:", ROTULO, "duracao ~%.0f s" % unreal._medir["ev"][-1][0])
