"""Recorta os 6 slots de jutsu da imagem do Felipe (Referencias/HUD/Slots_Jutsu_PNG/slots_original.png, já com
transparência de verdade) e apaga os números pintados na etiqueta (o número vira texto no jogo).
Cada slot sai numa caixa do mesmo tamanho com o círculo da arte no mesmo ponto (CENTRO), para o C++ usar uma medida só.
Saída: Referencias/HUD/Slots_Jutsu_PNG/pecas/T_JS_Slot<N>.png + Tools/hud/slots_jutsu_layout.json
Rodar: blender -b --factory-startup --python Tools/hud/recortar_slots_jutsu.py
"""
import bpy, json, os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from comum_hud import salvar, vizinhos

RAIZ = "/Users/luisfelipexavier/Desktop/Hype World"
ORIGEM = RAIZ + "/Referencias/HUD/Slots_Jutsu_PNG/slots_original.png"
SAIDA = RAIZ + "/Referencias/HUD/Slots_Jutsu_PNG/pecas"
LAYOUT = RAIZ + "/Tools/hud/slots_jutsu_layout.json"

# Medido (Blender + numpy): colunas de cada slot, centro do círculo e raio (px da imagem original)
COLUNAS = [(22, 371), (380, 727), (739, 1084), (1098, 1447), (1459, 1805), (1810, 2165)]
CENTROS_X = [204.5, 555.5, 913.0, 1276.0, 1634.5, 1995.5]
CENTRO_Y = 347.5
RAIO = 121.0
CAIXA_ESQ, CAIXA_DIR, CAIXA_CIMA, CAIXA_BAIXO = 230, 230, 200, 260   # caixa quadrada em volta do centro do círculo
ESCALA = 256.0 / 460.0   # textura 256x256 (potência de 2: pode ter mipmaps e não serrilha ao encolher)

im = bpy.data.images.load(ORIGEM)
w, h = im.size
px = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1].copy() * 255.0
bpy.data.images.remove(im)

os.makedirs(SAIDA, exist_ok=True)
layout = {"caixa": [CAIXA_ESQ + CAIXA_DIR, CAIXA_CIMA + CAIXA_BAIXO], "centro": [CAIXA_ESQ, CAIXA_CIMA], "raio": RAIO,
          "escala_textura": ESCALA, "slots": []}
for i, ((x0, x1), cx) in enumerate(zip(COLUNAS, CENTROS_X)):
    so = np.zeros_like(px)
    so[:, x0:x1 + 1] = px[:, x0:x1 + 1]     # só este slot (o vizinho não entra na caixa)
    # Etiqueta: pixels claros e sem cor (o número branco e o contorno cinza dele) viram o preto da etiqueta
    y0, y1 = int(CENTRO_Y + RAIO + 2), int(CENTRO_Y + RAIO + 120)
    ex0, ex1 = int(cx - 115), int(cx + 115)
    reg = so[y0:y1, ex0:ex1]
    rgb = reg[..., :3]
    mx, mn = rgb.max(-1), rgb.min(-1)
    sem_cor = (mx - mn) < 0.25 * np.maximum(mx, 1.0)
    branco = (mn > 215) & (reg[..., 3] > 128)
    ys, xs = np.where(branco)
    num_cx = float(np.percentile(xs, 50) + ex0) if len(xs) else cx
    num_cy = float((np.percentile(ys, 5) + np.percentile(ys, 95)) / 2 + y0) if len(ys) else CENTRO_Y + RAIO + 50
    escuro = (mx <= 18) & (reg[..., 3] > 200)
    fundo = np.median(rgb[escuro], axis=0) if escuro.any() else np.array([2.0, 2.0, 2.0])   # preto da etiqueta
    apagar = sem_cor & (mx > fundo.max() + 3) & (reg[..., 3] > 0)
    perto = branco.copy()   # em volta do número (4 px): a borda dele tem um leve tom azulado que o teste de "sem cor" não pega
    for _ in range(4):
        perto = vizinhos(perto)
    rgb[(apagar | (perto & (mx < 60))) & (mx > fundo.max() + 3)] = fundo   # mx < 60: não pinta a arte colorida
    # Caixa do mesmo tamanho para todos (preenche com transparente se passar da borda da imagem)
    bx0, by0 = int(round(cx - CAIXA_ESQ)), int(round(CENTRO_Y - CAIXA_CIMA))
    caixa = np.zeros((CAIXA_CIMA + CAIXA_BAIXO, CAIXA_ESQ + CAIXA_DIR, 4), np.float32)
    sx0, sy0 = max(bx0, 0), max(by0, 0)
    sx1, sy1 = min(bx0 + caixa.shape[1], w), min(by0 + caixa.shape[0], h)
    caixa[sy0 - by0:sy1 - by0, sx0 - bx0:sx1 - bx0] = so[sy0:sy1, sx0:sx1]
    nome = "T_JS_Slot%d" % (i + 1)
    salvar("%s/%s.png" % (SAIDA, nome), caixa, ESCALA)
    layout["slots"].append({"nome": nome, "numero_centro": [num_cx - bx0, num_cy - by0]})
    print("slot", i + 1, "número em", round(num_cx - bx0, 1), round(num_cy - by0, 1))

with open(LAYOUT, "w") as f:
    json.dump(layout, f, indent=1)
print("ok", LAYOUT)
