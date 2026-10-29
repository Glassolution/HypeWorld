"""Cópia da logo para a Unreal: recorta as letras (com margem) e põe transparência pela claridade (letras brancas sobre
fundo escuro). Não mexe na imagem de entrada.
Rodar: blender -b --factory-startup --python Tools/ui/recortar_logo.py -- <entrada> <saida.png>"""
import sys
import bpy
import numpy as np

src, dst = sys.argv[sys.argv.index("--") + 1:][:2]
img = bpy.data.images.load(src)
w, h = img.size
px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)   # linhas de baixo para cima
lum = px[..., :3] @ np.array([0.299, 0.587, 0.114], dtype=np.float32)
alpha = np.clip((lum - 0.25) / (0.60 - 0.25), 0.0, 1.0)
alpha = alpha * alpha * (3 - 2 * alpha)
ys, xs = np.where(alpha > 0.5)
m = 24
x0, x1 = max(xs.min() - m, 0), min(xs.max() + m + 1, w)
y0, y1 = max(ys.min() - m, 0), min(ys.max() + m + 1, h)
out = np.ones((y1 - y0, x1 - x0, 4), dtype=np.float32)
out[..., 3] = alpha[y0:y1, x0:x1]
res = bpy.data.images.new("logo", x1 - x0, y1 - y0, alpha=True)
res.pixels[:] = out.ravel()
res.filepath_raw = dst
res.file_format = "PNG"
res.save()
print("LOGO", x1 - x0, "x", y1 - y0, "recorte", x0, y0, x1, y1)
