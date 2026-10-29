"""Funções comuns (Blender + numpy) para recortar o bloco de vida das imagens do Felipe."""
import bpy, numpy as np

def carregar(caminho):
    im = bpy.data.images.load(caminho)
    w, h = im.size
    px = np.array(im.pixels[:], dtype=np.float32).reshape(h, w, 4)[::-1].copy()
    bpy.data.images.remove(im)
    return px[:, :, :3] * 255.0   # sRGB 0..255, linha 0 = topo

def salvar(caminho, rgba, escala=1.0):
    """rgba float 0..255 (H,W,4). escala < 1 reduz com média de área (sem serrilhar)."""
    h, w = rgba.shape[:2]
    if escala != 1.0:
        rgba = reduzir(rgba, max(1, round(w * escala)), max(1, round(h * escala)))
        h, w = rgba.shape[:2]
    im = bpy.data.images.new("tmp", w, h, alpha=True)
    im.colorspace_settings.name = "sRGB"
    a = rgba.astype(np.float32) / 255.0
    # cor pré-multiplicada desfeita nas bordas já vem certa (fazemos a média com peso do alfa em reduzir)
    im.pixels.foreach_set(a[::-1].reshape(-1))
    im.filepath_raw = caminho
    im.file_format = "PNG"
    im.save()
    bpy.data.images.remove(im)

def reduzir(rgba, nw, nh):
    """Redução por média de área, ponderada pelo alfa (as bordas transparentes não puxam a cor para preto)."""
    h, w = rgba.shape[:2]
    a = rgba[..., 3:4] / 255.0
    prem = np.concatenate([rgba[..., :3] * a, a], -1)
    # integral por eixo: soma das frações de pixel que caem em cada pixel novo
    def eixo(n_old, n_new):
        m = np.zeros((n_new, n_old), np.float32)
        s = n_old / n_new
        for i in range(n_new):
            x0, x1 = i * s, (i + 1) * s
            j0, j1 = int(np.floor(x0)), min(int(np.ceil(x1)), n_old)
            for j in range(j0, j1):
                m[i, j] = min(x1, j + 1) - max(x0, j)
            m[i] /= s
        return m
    my, mx = eixo(h, nh), eixo(w, nw)
    out = np.einsum("ij,jkc->ikc", my, prem)
    out = np.einsum("lk,ikc->ilc", mx, out)
    alpha = out[..., 3:4]
    cor = np.where(alpha > 1e-4, out[..., :3] / np.maximum(alpha, 1e-4), 0.0)
    return np.concatenate([cor, alpha * 255.0], -1)

def vizinhos(m):
    """Máscara dilatada em 1 pixel (8 vizinhos)."""
    d = m.copy()
    d[1:] |= m[:-1]; d[:-1] |= m[1:]; d[:, 1:] |= m[:, :-1]; d[:, :-1] |= m[:, 1:]
    d[1:, 1:] |= m[:-1, :-1]; d[:-1, :-1] |= m[1:, 1:]; d[1:, :-1] |= m[:-1, 1:]; d[:-1, 1:] |= m[1:, :-1]
    return d

def preencher(semente, permitido):
    """Inundação: cresce a semente só por pixels permitidos, até parar."""
    m = semente & permitido
    while True:
        n = vizinhos(m) & permitido
        if n.sum() == m.sum():
            return n
        m = n

def tirar_xadrez(rgb):
    """Fundo xadrez (branco ~254 e cinza ~238) ligado às bordas da imagem vira transparente; borda clara some e suaviza 1 px."""
    mx, mn = rgb.max(-1), rgb.min(-1)
    claro = (mn >= 226) & (mx - mn <= 8)
    borda = np.zeros_like(claro); borda[0] = borda[-1] = True; borda[:, 0] = borda[:, -1] = True
    fundo = preencher(borda, claro)
    # franja: pixels encostados no fundo que ainda são bem claros (mistura com o xadrez) também saem
    franja = vizinhos(vizinhos(fundo)) & ~fundo & (mn >= 200) & (mx - mn <= 20)
    fundo |= franja
    alfa = (~fundo).astype(np.float32)
    # suaviza 1 px
    p = np.pad(alfa, 1, mode="edge")
    alfa = sum(p[1 + dy: p.shape[0] - 1 + dy, 1 + dx: p.shape[1] - 1 + dx] for dy in (-1, 0, 1) for dx in (-1, 0, 1)) / 9.0
    alfa = np.minimum(alfa, (~fundo).astype(np.float32) * 0.5 + alfa * 0.5 + 0.5 * (~vizinhos(fundo)))
    return np.concatenate([rgb, (alfa * 255.0)[..., None]], -1)

def apagar_por_difusao(rgba, mascara, passos=400):
    """Apaga o que está em 'mascara' preenchendo com a média dos vizinhos (inpaint simples), mantendo o alfa."""
    out = rgba.copy()
    conhecido = ~mascara
    cor = out[..., :3].copy()
    cor[mascara] = 0
    for _ in range(passos):
        p = np.pad(cor, ((1, 1), (1, 1), (0, 0)), mode="edge")
        k = np.pad(conhecido.astype(np.float32), 1, mode="edge")
        soma = p[:-2, 1:-1] * k[:-2, 1:-1, None] + p[2:, 1:-1] * k[2:, 1:-1, None] + p[1:-1, :-2] * k[1:-1, :-2, None] + p[1:-1, 2:] * k[1:-1, 2:, None]
        peso = k[:-2, 1:-1] + k[2:, 1:-1] + k[1:-1, :-2] + k[1:-1, 2:]
        novo = mascara & (peso > 0)
        cor[novo] = soma[novo] / peso[novo][:, None]
        conhecido = conhecido | novo
        if conhecido.all():
            break
    # algumas passadas de suavização só dentro da máscara (tira degraus)
    for _ in range(20):
        p = np.pad(cor, ((1, 1), (1, 1), (0, 0)), mode="edge")
        media = (p[:-2, 1:-1] + p[2:, 1:-1] + p[1:-1, :-2] + p[1:-1, 2:]) / 4.0
        cor[mascara] = media[mascara]
    out[..., :3] = cor
    return out


def apagar_por_contraste(rgba, regiao_mask, limiar=28.0, margem=2):
    """Apaga texto preservando o desenho fraco do fundo: estima o fundo liso na região (difusão a partir da borda dela),
    e troca só os pixels que fogem muito dessa estimativa (texto, contorno, sombra)."""
    liso = apagar_por_difusao(rgba, regiao_mask)
    diff = np.abs(rgba[..., :3] - liso[..., :3]).max(-1)
    texto = (diff > limiar) & regiao_mask
    for _ in range(margem):
        texto = vizinhos(texto) & regiao_mask
    out = rgba.copy()
    out[texto, :3] = liso[texto, :3]
    return out
