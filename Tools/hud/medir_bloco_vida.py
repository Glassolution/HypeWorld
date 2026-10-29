"""Mede as partes do bloco de vida nas duas imagens do Felipe (Blender em segundo plano, numpy).
blender -b --factory-startup --python medir_bloco_vida.py -- <pasta com HUD_normal.png e HUD_batalha.png>
"""
import sys, bpy, numpy as np
pasta = sys.argv[sys.argv.index("--") + 1]

def carregar(nome):
    im = bpy.data.images.load(pasta + "/" + nome)
    w, h = im.size
    px = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1]   # linha 0 = topo
    im.colorspace_settings.name = "sRGB"
    return (px[:, :, :3] * 255.0).round().astype(np.int16)

def caixa(m):
    ys, xs = np.nonzero(m)
    return (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max()), int(m.sum())) if len(xs) else None

for nome in ("HUD_normal.png", "HUD_batalha.png"):
    a = carregar(nome)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    mx, mn = a.max(-1), a.min(-1)
    print("=====", nome, a.shape)
    vals, cnt = np.unique(a[:60, :60].reshape(-1, 3), axis=0, return_counts=True)
    top = np.argsort(-cnt)[:6]
    print("cores do canto (xadrez):", [(tuple(vals[i]), int(cnt[i])) for i in top])
    rosa = (r > 180) & (g < 90) & (b > 60) & (b < 150)
    print("vida rosa:", caixa(rosa))
    azul_claro = (b > 200) & (g > 150) & (r < 150) & (b - r > 70)
    print("azul claro (anel/chakra):", caixa(azul_claro))
    cinza = (mx - mn < 14) & (mn > 80) & (mx < 140)
    print("cinza médio:", caixa(cinza))
    preto = mx < 45
    print("preto:", caixa(preto))
    amarelo = (r > 200) & (g > 160) & (b < 90)
    print("amarelo (moeda):", caixa(amarelo))
    # perfil por coluna/linha das máscaras para achar limites
    for nm, m in (("rosa", rosa), ("azul", azul_claro), ("preto", preto), ("cinza", cinza)):
        cols = np.nonzero(m.sum(0) > 3)[0]; rows = np.nonzero(m.sum(1) > 3)[0]
        # segmentos contínuos em x
        segs = []
        if len(cols):
            ini = cols[0]; ant = cols[0]
            for c in cols[1:]:
                if c > ant + 3: segs.append((int(ini), int(ant))); ini = c
                ant = c
            segs.append((int(ini), int(ant)))
        print("  segs x", nm, segs[:8])
        segs = []
        if len(rows):
            ini = rows[0]; ant = rows[0]
            for c in rows[1:]:
                if c > ant + 3: segs.append((int(ini), int(ant))); ini = c
                ant = c
            segs.append((int(ini), int(ant)))
        print("  segs y", nm, segs[:8])
