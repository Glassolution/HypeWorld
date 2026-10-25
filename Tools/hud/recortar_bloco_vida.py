"""Recorta o bloco de vida das imagens do Felipe em peças para o HUD (Blender em segundo plano, numpy):
fundo xadrez -> transparente; textos pintados (vida, chakra, nome, nível, moedas) e o arco azul apagados — o jogo desenha
por cima com a Montserrat e os valores de verdade. Saída: PNGs em <pasta>/pecas e as posições em bloco_vida_layout.json
(coordenadas em pixels da imagem normal, inteira).
As regiões aproximadas foram medidas olhando as imagens com grade de 100 px; dentro delas o recorte é automático.
blender -b --factory-startup --python recortar_bloco_vida.py -- <pasta com HUD_normal.png e HUD_batalha.png>
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import comum_hud as C

PASTA = sys.argv[sys.argv.index("--") + 1]
SAIDA = os.path.join(PASTA, "pecas")
os.makedirs(SAIDA, exist_ok=True)
ESCALA_PNG = 0.36   # as peças saem em ~1,5x do tamanho na tela (1080p), sem serrilhar
layout = {}

# Regiões aproximadas [x0, y0, x1, y1] (medidas na grade)
NORMAL = dict(vida=[588, 230, 1914, 394], chakra=[584, 398, 1441, 536], nome=[560, 95, 1230, 232], texto_vida=[662, 256, 1030, 364],
              texto_chakra=[1172, 428, 1398, 510], selo=[258, 530, 448, 700], moeda=[558, 568, 692, 696], texto_moedas=[694, 570, 1030, 705],
              anel=[85, 115, 625, 700])
BATALHA = dict(vida=[560, 248, 1845, 402], chakra=[558, 402, 1400, 542], texto_chakra=[1022, 430, 1342, 508],
               icones=[[548, 546, 713, 700], [713, 546, 876, 700]])

N = C.tirar_xadrez(C.carregar(os.path.join(PASTA, "HUD_normal.png")))
B = C.tirar_xadrez(C.carregar(os.path.join(PASTA, "HUD_batalha.png")))
H, W = N.shape[:2]
yy, xx = np.mgrid[0:H, 0:W]


def caixa(m):
    ys, xs = np.nonzero(m)
    return [int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1]


def dilatar(m, n):
    for _ in range(n):
        m = C.vizinhos(m)
    return m


def dentro(shape, r):
    m = np.zeros(shape[:2], bool)
    m[r[1]:r[3], r[0]:r[2]] = True
    return m


def componente(img, r):
    """Pixels visíveis ligados ao centro da região r (separa a barra do vizinho que encosta na caixa)."""
    vis = (img[..., 3] > 40) & dentro(img.shape, r)
    cx, cy = (r[0] + r[2]) // 2, (r[1] + r[3]) // 2
    sem = np.zeros_like(vis)
    # semente: pixel visível mais perto do centro na linha do meio
    linha = np.nonzero(vis[cy])[0]
    sem[cy, linha[np.argmin(np.abs(linha - cx))]] = True
    return C.preencher(sem, vis)


def recorte(img, m, pad=2):
    """Copia só os pixels da máscara (resto transparente) e devolve (imagem cortada, caixa)."""
    x0, y0, x1, y1 = caixa(m)
    x0, y0, x1, y1 = max(x0 - pad, 0), max(y0 - pad, 0), min(x1 + pad, img.shape[1]), min(y1 + pad, img.shape[0])
    out = img[y0:y1, x0:x1].copy()
    mm = dilatar(m, 1)[y0:y1, x0:x1]
    out[..., 3] = np.where(mm, out[..., 3], 0)
    return out, [x0, y0, x1, y1]


def sem_borda_clara(img, limite_min=150):
    """Tira a beirada clara/acinzentada que sobrou do xadrez em volta das barras (pontilhado)."""
    rgb = img[..., :3]
    clara = (rgb.min(-1) > limite_min) & (rgb.max(-1) - rgb.min(-1) < 40)
    borda = C.vizinhos(img[..., 3] < 20)
    for _ in range(6):
        borda = C.vizinhos(borda)
    img[clara & borda, 3] = 0
    return img


def salvar(nome, img, ret):
    C.salvar(os.path.join(SAIDA, nome + ".png"), img, ESCALA_PNG)
    layout[nome] = [float(v) for v in ret]


def apagar_texto(img, regiao, cond, dil):
    """Apaga o texto pintado (pixels de 'cond' na região), preenchendo com a cor em volta."""
    rgb = img[..., :3]
    t = cond(rgb.max(-1), rgb.min(-1)) & dentro(img.shape, regiao) & (img[..., 3] > 128)
    if t.any():
        x0, y0, x1, y1 = caixa(t)
        reg = dentro(img.shape, [x0 - dil - 8, y0 - dil - 8, x1 + dil + 8, y1 + dil + 8]) & (img[..., 3] > 128)
        img[:] = C.apagar_por_contraste(img, reg & dilatar(t, dil + 6), limiar=10.0, margem=3)
        return caixa(t)
    return None


# ================= Imagem normal =================
# Nome e número das moedas saem (o jogo escreve)
for k in ("nome", "texto_moedas"):
    x0, y0, x1, y1 = NORMAL[k]
    N[y0:y1, x0:x1, 3] = 0

# Anel: acha o círculo pelo arco azul, apaga o arco (vira a cor do trilho); o jogo desenha o arco de XP
azul = (N[..., 2] > 200) & (N[..., 1] > 150) & (N[..., 0] < 150) & (N[..., 2] - N[..., 0] > 70) & dentro(N.shape, NORMAL["anel"])
ys, xs = np.nonzero(azul)
A = np.c_[2 * xs, 2 * ys, np.ones(len(xs))]
cx, cy, _ = np.linalg.lstsq(A, xs ** 2.0 + ys ** 2.0, rcond=None)[0]
dist = np.hypot(xs - cx, ys - cy)
r_in, r_out = np.percentile(dist, 0.5), np.percentile(dist, 99.5)
cor_arco = np.median(N[ys, xs, :3], 0)
dd = np.hypot(xx - cx, yy - cy)
anel = (dd >= r_in - 2) & (dd <= r_out + 2)
trilho = anel & ~dilatar(azul, 3) & (N[..., 3] > 200) & (N[..., :3].max(-1) < 90)
cor_trilho = np.median(N[trilho][:, :3], 0)
azulidade = np.clip((N[..., 2] - N[..., 0] - 20.0) / 120.0, 0, 1) * dilatar(anel, 2)
N[..., :3] = N[..., :3] * (1 - azulidade[..., None]) + cor_trilho * azulidade[..., None]
ang = (np.degrees(np.arctan2(-(xs - cx), ys - cy)) + 360) % 360   # 0 = embaixo, cresce pela esquerda (como o C++)
layout["anel"] = {"centro": [float(cx), float(cy)], "raio_interno": float(r_in), "raio_externo": float(r_out),
                  "cor": [float(v) for v in cor_arco], "cor_trilho": [float(v) for v in cor_trilho],
                  "arco_da_imagem_graus": [float(ang.min()), float(ang.max())]}
print("anel", layout["anel"])

# Selo de nível: círculo preto; os números saem; peça própria (vai por cima do arco)
x0, y0, x1, y1 = NORMAL["selo"]
# círculo preto: largura pelos pixels pretos abaixo do anel; o centro fica um raio acima do ponto mais baixo
preto = (N[..., :3].max(-1) < 50) & (N[..., 3] > 200) & dentro(N.shape, NORMAL["selo"])
pb = caixa(preto & (yy > cy + r_out - 10))
sr = (pb[2] - pb[0]) / 2
sx, sy = (pb[0] + pb[2]) / 2, pb[3] - sr
circ = np.hypot(xx - sx, yy - sy) < sr - 6
dig = (N[..., :3].min(-1) > 130) & (N[..., 3] > 200) & circ
digc = caixa(dig)
# miolo do selo: preto liso (cor mediana do próprio selo), só a borda original fica
cor_selo = np.median(N[circ & (N[..., :3].max(-1) < 60)][:, :3], 0)
N[np.hypot(xx - sx, yy - sy) < sr - 12, :3] = cor_selo
selo_m = np.hypot(xx - sx, yy - sy) <= sr + 4
img, ret = recorte(N, selo_m & (N[..., 3] > 10))
salvar("Selo", img, ret)
layout["selo"] = {"centro": [float(sx), float(sy)], "raio": float(sr), "digitos": digc}
print("selo", layout["selo"])

# Moeda (só fora da batalha)
m = componente(N, NORMAL["moeda"])
img, ret = recorte(N, m)
salvar("Moeda", img, ret)
N[m | dilatar(m, 2) & dentro(N.shape, NORMAL["moeda"]), 3] = 0

# Vida: barra cheia (texto apagado) e vazia (escurecida)
m = componente(N, NORMAL["vida"])
img, ret = recorte(N, m)
off = np.array([ret[0], ret[1], ret[0], ret[1]])
# Texto da vida: cada linha da barra tem a sua cor (degradê de cima para baixo); a referência de cada linha vem de um
# trecho sem texto logo à direita. Troca só o que foge dessa cor (número, contorno, sombra larga); bordas da barra ficam.
r_t = list(np.array(NORMAL["texto_vida"]) - off)
t = (img[..., :3].min(-1) > 165) & dentro(img.shape, r_t) & (img[..., 3] > 128)
tv = caixa(t)
D = 22
ref0, ref1 = tv[2] + 60, tv[2] + 360
interior = img[..., 3] > 200
for _ in range(6):
    interior = interior & ~C.vizinhos(~interior)
refs = np.zeros_like(img[..., :3])
troca = np.zeros(img.shape[:2], bool)
x0_, x1_ = max(tv[0] - D, 0), tv[2] + D
for y in range(max(tv[1] - D, 0), min(tv[3] + D, img.shape[0])):
    linha = img[y, ref0:ref1]
    ok = linha[..., 3] > 200
    if ok.sum() < 20:
        continue
    ref = np.median(linha[ok][:, :3], 0)
    refs[y] = ref
    troca[y, x0_:x1_] = np.abs(img[y, x0_:x1_, :3] - ref).max(-1) > 8
troca = C.vizinhos(C.vizinhos(troca)) & interior & dentro(img.shape, [x0_, tv[1] - D, x1_, tv[3] + D])
img[troca, :3] = refs[troca]
# suaviza a emenda só na área trocada
area = dentro(img.shape, [tv[0] - D, tv[1] - D, tv[2] + D, tv[3] + D]) & interior
for _ in range(3):
    p_ = np.pad(img[..., :3], ((1, 1), (1, 1), (0, 0)), mode="edge")
    media = (p_[:-2, 1:-1] + p_[2:, 1:-1] + p_[1:-1, :-2] + p_[1:-1, 2:] + p_[1:-1, 1:-1]) / 5.0
    img[area, :3] = media[area]
layout["texto_vida"] = [float(v) for v in (np.array(tv) + off)]
sem_borda_clara(img)
salvar("Vida_Cheia", img, ret)
vz = img.copy(); vz[..., :3] = vz[..., :3] * np.array([0.30, 0.15, 0.19]) + np.array([14.0, 6.0, 9.0])
salvar("Vida_Vazia", vz, ret)
N[dilatar(m, 2) & dentro(N.shape, NORMAL["vida"]), 3] = 0

# Chakra normal (cinza): "100%" sai
m = componente(N, NORMAL["chakra"])
img, ret = recorte(N, m)
CHAKRA_N = ret
off = np.array([ret[0], ret[1], ret[0], ret[1]])
tc = apagar_texto(img, list(np.array(NORMAL["texto_chakra"]) - off), lambda mx, mn: mn > 165, 5)
layout["texto_chakra_normal"] = [float(v) for v in (np.array(tc) + off)]
sem_borda_clara(img, 175)
salvar("Chakra_Normal_Cheia", img, ret)
cz = img.copy(); cz[..., :3] = cz[..., :3] * 0.33
salvar("Chakra_Normal_Vazia", cz, ret)
N[dilatar(m, 2) & dentro(N.shape, NORMAL["chakra"]), 3] = 0

# Base: o resto (anel com trilho, retrato, caixa de voz, selo com número apagado)
TUDO = caixa(N[..., 3] > 10)
salvar("Base", N[TUDO[1]:TUDO[3], TUDO[0]:TUDO[2]], TUDO)
vida_n = caixa(componente(C.tirar_xadrez(C.carregar(os.path.join(PASTA, "HUD_normal.png"))), NORMAL["vida"]))

# ================= Imagem de batalha =================
vida_b = caixa(componente(B, BATALHA["vida"]))
esc = (vida_n[3] - vida_n[1]) / (vida_b[3] - vida_b[1])   # tamanho batalha -> normal
m = componente(B, BATALHA["chakra"])
img, retb = recorte(B, m)
off = np.array([retb[0], retb[1], retb[0], retb[1]])
tb = apagar_texto(img, list(np.array(BATALHA["texto_chakra"]) - off), lambda mx, mn: mx < 115, 4)
x0, y0, x1, y1 = BATALHA["texto_chakra"]
reg = B[y0:y1, x0:x1].reshape(-1, 4)
cor_txt = np.median(reg[(reg[:, :3].max(-1) < 115) & (reg[:, 3] > 128)][:, :3], 0)
# a barra azul ocupa exatamente a caixa da cinza
sx_, sy_ = (CHAKRA_N[2] - CHAKRA_N[0]) / (retb[2] - retb[0]), (CHAKRA_N[3] - CHAKRA_N[1]) / (retb[3] - retb[1])
layout["texto_chakra_batalha"] = [CHAKRA_N[0] + (tb[0]) * sx_, CHAKRA_N[1] + tb[1] * sy_, CHAKRA_N[0] + tb[2] * sx_, CHAKRA_N[1] + tb[3] * sy_]
layout["cor_texto_chakra_batalha"] = [float(v) for v in cor_txt]
sem_borda_clara(img)
salvar("Chakra_Batalha_Cheia", img, CHAKRA_N)
cz = img.copy(); cz[..., :3] = cz[..., :3] * np.array([0.22, 0.27, 0.36])   # azul bem escuro: o texto do jogo é branco
salvar("Chakra_Batalha_Vazia", cz, CHAKRA_N)

# Ícones (batalha): a barrinha de baixo vira só o trilho; o jogo desenha o preenchimento
moeda = layout["Moeda"]
base_x = moeda[0]
for i, r in enumerate(BATALHA["icones"]):
    m = componente(B, r)
    img, ret = recorte(B, m)
    sat = img[..., :3].max(-1) - img[..., :3].min(-1)
    h = img.shape[0]
    if i == 0:   # as duas peças têm o mesmo desenho: a faixa da barra é medida no 1º ícone (no 2º a arte azul confunde)
        linhas = np.nonzero((sat > 70).sum(1) > img.shape[1] * 0.3)[0]
        linhas = linhas[linhas > h * 0.72]
        ly0, ly1 = int(linhas.min()), int(linhas.max()) + 1
    cols = np.nonzero((sat[ly0:ly1] > 40).sum(0) > (ly1 - ly0) * 0.5)[0]
    bx0, bx1 = int(cols.min()), int(cols.max()) + 1
    cores = img[ly0:ly1, bx0:bx1, :3].reshape(-1, 3)
    brilho = cores.sum(-1)
    cheia = np.median(cores[brilho >= np.percentile(brilho, 75)], 0)
    vazia = np.median(cores[brilho <= np.percentile(brilho, 25)], 0)
    img[ly0:ly1, bx0:bx1, :3] = vazia
    w_, h_ = (ret[2] - ret[0]) * esc, (ret[3] - ret[1]) * esc
    gx = base_x + (ret[0] - BATALHA["icones"][0][0]) * esc
    gy = (moeda[1] + moeda[3]) / 2 - h_ / 2
    salvar("Icone%d" % (i + 1), img, [gx, gy, gx + w_, gy + h_])
    layout["Icone%d_barra" % (i + 1)] = [gx + bx0 * esc, gy + ly0 * esc, gx + bx1 * esc, gy + ly1 * esc]
    layout["Icone%d_cores" % (i + 1)] = [[float(v) for v in cheia], [float(v) for v in vazia]]

with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "bloco_vida_layout.json"), "w") as f:
    json.dump(layout, f, indent=1)
for k, v in layout.items():
    print(k, v)
print("PRONTO")
