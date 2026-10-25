"""Prévia do bloco montado com as peças (como o jogo monta), vida 60%, chakra 40%, XP 70%, nos dois modos.
blender -b --factory-startup --python previa_bloco_vida.py -- <pasta das peças> <saída.png>
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np, comum_hud as C
pasta, saida = sys.argv[sys.argv.index("--") + 1:][:2]
L = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "bloco_vida_layout.json")))
K = 0.36
def png(n):
    import bpy
    im = bpy.data.images.load(os.path.join(pasta, n + ".png")); w, h = im.size
    a = np.array(im.pixels[:], np.float32).reshape(h, w, 4)[::-1] * 255.0; bpy.data.images.remove(im); return a
def por(tela, img, r, corte=1.0):
    x0, y0 = int(round(r[0] * K)), int(round(r[1] * K))
    h, w = img.shape[:2]
    w = int(round(w * corte)); img = img[:, :w]
    a = img[..., 3:4] / 255.0
    reg = tela[y0:y0 + h, x0:x0 + w]
    reg[..., :3] = img[..., :3] * a + reg[..., :3] * (1 - a)
    reg[..., 3:4] = np.maximum(reg[..., 3:4], img[..., 3:4])
quadros = []
for batalha in (False, True):
    tela = np.zeros((int(760 * K), int(1950 * K), 4), np.float32); tela[..., :3] = [60, 80, 70]; tela[..., 3] = 255
    por(tela, png("Base"), L["Base"])
    an = L["anel"]; cx, cy = an["centro"][0] * K, an["centro"][1] * K
    yy, xx = np.mgrid[0:tela.shape[0], 0:tela.shape[1]]
    d = np.hypot(xx - cx, yy - cy); ang = (np.degrees(np.arctan2(-(xx - cx), yy - cy)) + 360) % 360
    arco = (d >= an["raio_interno"] * K) & (d <= an["raio_externo"] * K) & (ang <= 0.7 * 360)
    tela[arco, :3] = an["cor"]
    por(tela, png("Selo"), L["Selo"])
    por(tela, png("Vida_Vazia"), L["Vida_Vazia"]); por(tela, png("Vida_Cheia"), L["Vida_Cheia"], 0.6)
    m = "Batalha" if batalha else "Normal"
    por(tela, png("Chakra_%s_Vazia" % m), L["Chakra_%s_Vazia" % m]); por(tela, png("Chakra_%s_Cheia" % m), L["Chakra_%s_Cheia" % m], 0.4)
    if batalha:
        for i in (1, 2):
            por(tela, png("Icone%d" % i), L["Icone%d" % i])
            b = L["Icone%d_barra" % i]; x0, y0, x1, y1 = [int(round(v * K)) for v in b]
            tela[y0:y1, x0:x0 + int((x1 - x0) * 0.5), :3] = L["Icone%d_cores" % i][0]
    else:
        por(tela, png("Moeda"), L["Moeda"])
    for k in ("texto_vida", "texto_chakra_batalha" if batalha else "texto_chakra_normal"):   # contorno onde o texto vai
        x0, y0, x1, y1 = [int(round(v * K)) for v in L[k]]
        tela[y0, x0:x1, :3] = tela[y1, x0:x1, :3] = [255, 255, 0]; tela[y0:y1, x0, :3] = tela[y0:y1, x1, :3] = [255, 255, 0]
    quadros.append(tela)
C.salvar(saida, np.concatenate(quadros, 0))
print("PRONTO")
