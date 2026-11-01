# Indra modular (01/11/2026): separa o SK_Indra em cabeça careca (SK_Indra_Head), cabelo (SK_Hair_Indra) e corpo
# (SK_Outfit_Indra), todos no rig original do Indra_Referencia.fbx (sem mexer em osso nem bind pose).
# - O Indra é um "rip" oco: sem cabelo não tem topo de crânio. O topo é fechado com uma cúpula (triangulação da borda
#   vista de cima, levantada sobre um elipsoide e corrigida para encostar na borda), sempre por dentro do cabelo.
# - Pescoço cortado no plano CORTE_Z: o anel da borda é o padrão de corte de todos os trajes (anel_pescoco.json).
# - Normais: as faces originais recebem as normais do original (Data Transfer); a borda da cúpula copia as do rosto.
# Uso: blender -b --factory-startup --python Tools/customization/indra_cabeca_modular.py -- [--fotos <pasta>] [--sem-fbx]
import bpy, bmesh, json, math, sys
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix, geometry
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

ROOT = Path(__file__).resolve().parents[2]
REF = ROOT / 'Docs/PadraoPersonagem/Indra_Referencia.fbx'
OUT = ROOT / 'SourceAssets/Customization/IndraModular'
args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
FOTOS = Path(args[args.index('--fotos') + 1]) if '--fotos' in args else None
SEM_FBX = '--sem-fbx' in args

CORTE_Z = 1.485        # m: abaixo do queixo (1,519) e da mandíbula, acima do trapézio; nuca original inteira aqui
PASSO = 0.009          # m: espaçamento dos vértices novos da cúpula
FOLGA_CABELO = 0.004   # m: a cúpula fica pelo menos isso por dentro do cabelo
MAT_ROSTO, MAT_PELE, MAT_GOLA = 4, 2, 3
MATS_OLHOS, MATS_CABELO = (0, 1), (5, 6)
NOME_CRANIO = 'M_Indra_Cranio'

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(REF))
rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
src = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
rig.data.pose_position = 'REST'
bpy.context.view_layer.update()
M = src.matrix_world.copy(); Mi = M.inverted()
gnames = {g.index: g.name for g in src.vertex_groups}
HEAD_BONE = next(b.name for b in rig.data.bones if b.name.startswith('9ind00t0-head_'))

def bm_mundo():
    bm = bmesh.new(); bm.from_mesh(src.data); bm.transform(M); return bm

def dominante(bm, f, dl):
    soma = {}
    for v in f.verts:
        for g, w in v[dl].items():
            soma[g] = soma.get(g, 0) + w
    return gnames[max(soma, key=soma.get)] if soma else ''

def classificar(bm):
    """'cabelo' | 'cabeca' | 'corpo' por face (antes do corte)."""
    dl = bm.verts.layers.deform.active
    out = {}
    for f in bm.faces:
        c = f.calc_center_median()
        if f.material_index in MATS_CABELO or (f.material_index == MAT_PELE and 'hair' in dominante(bm, f, dl)):
            out[f.index] = 'cabelo'
        elif f.material_index in MATS_OLHOS or f.material_index == MAT_ROSTO or \
                (f.material_index == MAT_PELE and c.z > 1.38 and abs(c.x) < 0.15 and abs(c.y) < 0.15):
            out[f.index] = 'cabeca'
        else:
            out[f.index] = 'corpo'
    return out

def manter(bm, faces_ok):
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f not in faces_ok], context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')

def cortar(bm, faces, manter_acima):
    geom = list({v for f in faces for v in f.verts}) + list({e for f in faces for e in f.edges}) + list(faces)
    bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-7, plane_co=(0, 0, CORTE_Z), plane_no=(0, 0, 1),
                           clear_inner=manter_acima, clear_outer=not manter_acima)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')

def lacos_borda(bm):
    adj = {}
    for e in bm.edges:
        if e.is_boundary:
            a, b = e.verts; adj.setdefault(a, []).append(b); adj.setdefault(b, []).append(a)
    vistos, lacos = set(), []
    for v0 in adj:
        if v0 in vistos or len(adj[v0]) != 2:
            continue
        laco, prev, cur = [v0], None, v0
        vistos.add(v0)
        while True:
            nxt = [n for n in adj[cur] if n is not prev and len(adj[n]) == 2]
            if not nxt or nxt[0] is v0:
                break
            if nxt[0] in vistos:
                laco = None; break
            prev, cur = cur, nxt[0]; laco.append(cur); vistos.add(cur)
        if laco and len(laco) > 2:
            lacos.append(laco)
    return lacos

# ---------------------------------------------------------------- partes
base = bm_mundo(); base.faces.ensure_lookup_table()
cls = classificar(base)

def parte(nome):
    bm = bm_mundo(); bm.faces.ensure_lookup_table()
    manter(bm, {bm.faces[i] for i, c in cls.items() if (c == nome or (nome == 'corpo' and c == 'cabeca'))})
    return bm

bm_cabelo = parte('cabelo')
bm_cabeca = parte('cabeca')
bm_corpo = parte('corpo')

# corpo: tira a parte da cabeça acima do corte (o resto do decote em V fica com o corpo)
dl = bm_corpo.verts.layers.deform.active
cab_corpo = [f for f in bm_corpo.faces if f.material_index in MATS_OLHOS + (MAT_ROSTO,) or
             (f.material_index == MAT_PELE and f.calc_center_median().z > 1.38 and abs(f.calc_center_median().x) < 0.15
              and abs(f.calc_center_median().y) < 0.15 and 'hair' not in dominante(bm_corpo, f, dl))]
cortar(bm_corpo, cab_corpo, manter_acima=False)

# cabeça: solda os triângulos soltos do rip e corta o pescoço no plano
bmesh.ops.remove_doubles(bm_cabeca, verts=list(bm_cabeca.verts), dist=1e-5)
cortar(bm_cabeca, list(bm_cabeca.faces), manter_acima=True)
bmesh.ops.remove_doubles(bm_cabeca, verts=list(bm_cabeca.verts), dist=1e-6)

lacos = lacos_borda(bm_cabeca)
resumo = []
for l in lacos:
    zs = [v.co.z for v in l]
    ys = [v.co.y for v in l]
    resumo.append((len(l), round(min(zs), 3), round(max(zs), 3), round(float(np.mean(zs)), 3), round(min(ys), 3), round(max(ys), 3)))
print('LACOS', sorted(resumo, reverse=True)[:12])
# anel = borda mais baixa (pescoço)
anel = min(lacos, key=lambda l: np.mean([v.co.z for v in l]))
print('ANEL', len(anel), 'acima do corte:', sum(1 for v in anel if v.co.z > CORTE_Z + 1e-4), 'x', round(min(v.co.x for v in anel), 3), round(max(v.co.x for v in anel), 3), 'y', round(min(v.co.y for v in anel), 3), round(max(v.co.y for v in anel), 3))
# O topo aberto fica entre duas cascas sobrepostas do rip: a máscara do rosto (borda de cima = linha da testa) e o
# cilindro de trás da cabeça (borda de cima = altura do alto da orelha). A borda do furo = arco de cima da testa +
# arco de cima do cilindro; nas laterais os dois cantos quase se tocam e a cúpula faz a ponte.
def arco(laco, cond):
    n = len(laco); ok = [cond(v) for v in laco]
    if all(ok):
        return list(laco)
    ini = next(i for i in range(n) if ok[i] and not ok[i - 1])
    out = []
    i = ini
    while ok[i % n]:
        out.append(laco[i % n]); i += 1
    return out
mascara = next(l for l in lacos if min(v.co.y for v in l) < -0.08 and max(v.co.z for v in l) > 1.69)
cilindro = next(l for l in lacos if max(v.co.y for v in l) > 0.06 and max(v.co.z for v in l) > 1.64 and l is not anel)
testa = arco(mascara, lambda v: v.co.z > 1.655)
nuca = arco(cilindro, lambda v: v.co.y > -0.015 and v.co.z > 1.645)
# testa vai de um lado ao outro pela frente; nuca tem que voltar pelo lado oposto (por trás)
if (nuca[0].co - testa[-1].co).length > (nuca[-1].co - testa[-1].co).length:
    nuca.reverse()
topo = testa + nuca
print('TOPO testa', len(testa), 'nuca', len(nuca), 'pontas', [tuple(round(c, 3) for c in v.co) for v in (testa[0], testa[-1], nuca[0], nuca[-1])])
# anel limpo: todo vértice da borda do pescoço exatamente no plano de corte
for v in anel:
    v.co.z = CORTE_Z

anel_pts = [[float(c) for c in v.co] for v in anel]

# ---------------------------------------------------------------- cúpula
hair_tris = []
bm_cabelo.faces.ensure_lookup_table()
hair_bvh = BVHTree.FromBMesh(bm_cabelo)
bxy = np.array([(v.co.x, v.co.y) for v in topo]); bz = np.array([v.co.z for v in topo])
centro = Vector((float(bxy[:, 0].mean()), float(bxy[:, 1].mean()), float(bz.min()) - 0.02))
TOPO_Z = 1.715   # topo do crânio anime (queixo 1,519, olhos 1,618): olhos no meio da cabeça (1,735 ficou alto/quadrado)
# Perfil da cúpula: em cada direção (vista de cima) uma curva de Hermite no plano vertical, da borda até o topo.
# Na borda a curva sai com a mesma inclinação da superfície original (parede/testa), no topo chega na horizontal:
# assim não há vinco na emenda.
eixo = Vector((centro.x, centro.y))
dados_borda = []
nb_ = len(topo)
for i, v in enumerate(topo):
    tang = (topo[(i + 1) % nb_].co - topo[i - 1].co).normalized()
    nrm = sum((f.normal for f in v.link_faces), Vector()).normalized()
    meio = sum((f.calc_center_median() for f in v.link_faces), Vector()) / max(len(v.link_faces), 1)
    o = tang.cross(nrm).normalized()
    if o.dot(v.co - meio) < 0:
        o = -o
    rad = Vector((v.co.x - eixo.x, v.co.y - eixo.y))
    rho = rad.length; rad.normalize()
    t_rho, t_z = o.x * rad.x + o.y * rad.y, max(o.z, 0.35)
    n2 = math.hypot(t_rho, t_z)
    dados_borda.append((math.atan2(rad.y, rad.x), rho, v.co.z, t_rho / n2, t_z / n2))
dados_borda.sort()
def borda_em(theta):
    th = [d[0] for d in dados_borda]
    j = int(np.searchsorted(th, theta)) % len(th)
    d0, d1 = dados_borda[j - 1], dados_borda[j]
    span = (d1[0] - d0[0]) % (2 * math.pi) or 1e-9
    w = ((theta - d0[0]) % (2 * math.pi)) / span
    return [d0[k] * (1 - w) + d1[k] * w for k in range(1, 5)]
def z_perfil(x, y):
    dx, dy = x - eixo.x, y - eixo.y
    rho = math.hypot(dx, dy); theta = math.atan2(dy, dx)
    rb_, zb_, tr, tz = borda_em(theta)
    L = math.hypot(rb_, TOPO_Z - zb_)
    def P(t):
        h00 = 2 * t**3 - 3 * t**2 + 1; h10 = t**3 - 2 * t**2 + t; h01 = -2 * t**3 + 3 * t**2; h11 = t**3 - t**2
        return (h00 * rb_ + h10 * tr * L + h01 * 0.0 + h11 * (-1.0) * L,
                h00 * zb_ + h10 * tz * L + h01 * TOPO_Z + h11 * 0.0 * L)
    lo, hi = 0.0, 1.0
    for _ in range(40):
        mid = (lo + hi) / 2
        if P(mid)[0] > rho:
            lo = mid
        else:
            hi = mid
    return P((lo + hi) / 2)[1]

# pontos internos numa grade dentro da borda (vista de cima)
poly = [Vector((x, y)) for x, y in bxy]
pts = [Vector((x, y)) for x, y in bxy]
xs = np.arange(bxy[:, 0].min() + PASSO / 2, bxy[:, 0].max(), PASSO)
ys = np.arange(bxy[:, 1].min() + PASSO / 2, bxy[:, 1].max(), PASSO * 0.866)
def dentro(p):
    c = False; n = len(poly)
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        if (a.y > p.y) != (b.y > p.y) and p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x:
            c = not c
    return c
def dist_borda(p):
    return min(geometry.intersect_point_line(p, poly[i], poly[(i + 1) % len(poly)])[0].__sub__(p).length
               if 0 <= geometry.intersect_point_line(p, poly[i], poly[(i + 1) % len(poly)])[1] <= 1
               else min((poly[i] - p).length, (poly[(i + 1) % len(poly)] - p).length) for i in range(len(poly)))
for j, y in enumerate(ys):
    for x in xs + (PASSO / 2 if j % 2 else 0):
        p = Vector((x, y))
        if dentro(p) and dist_borda(p) > PASSO * 0.6:
            pts.append(p)
nb = len(topo)
edges = [(i, (i + 1) % nb) for i in range(nb)]
vout, eout, fout, vorig, _, _ = geometry.delaunay_2d_cdt(pts, edges, [], 1, 1e-7)  # 1 = só o interior
# vorig liga cada vértice de saída ao ponto de entrada
mapa = [o[0] if o else None for o in vorig]
dl = bm_cabeca.verts.layers.deform.active
novos = []
for k, co2 in enumerate(vout):
    src_i = mapa[k]
    if src_i is not None and src_i < nb:
        novos.append(topo[src_i])
    else:
        v = bm_cabeca.verts.new((co2.x, co2.y, z_perfil(co2.x, co2.y)))
        novos.append(v)
interior = [v for v in novos if v not in set(topo)]
borda = set(topo)
mat_cranio = len(src.data.materials)
faces_cap = []
for f in fout:
    vs = [novos[i] for i in f]
    if len(set(vs)) < 3:
        continue
    try:
        nf = bm_cabeca.faces.new(vs)
    except ValueError:
        continue
    nf.material_index = mat_cranio; nf.smooth = True
    faces_cap.append(nf)
bm_cabeca.normal_update()
# orientação: todas as faces da cúpula no mesmo sentido (o da triangulação), virado para fora pela maioria
para_fora = sum(f.normal.dot(f.calc_center_median() - centro) for f in faces_cap)
if para_fora < 0:
    for f in faces_cap:
        f.normal_flip()

viz = {v: [e.other_vert(v) for e in v.link_edges if any(lf in faces_cap for lf in e.link_faces)] for v in interior}

# limite do cabelo: o crânio fica por dentro da superfície EXTERNA do cabelo (raio de fora para o centro; mechas
# soltas por dentro da cabeça não contam). O limite por vértice é suavizado só para baixo (envelope inferior) para
# a cúpula não pegar o relevo das pontas do cabelo.
def raio_cabelo(v):
    dirv = (v.co - centro).normalized()
    hit = hair_bvh.ray_cast(centro + dirv * 0.6, -dirv, 0.6)
    return (0.6 - hit[3] - FOLGA_CABELO) if hit[0] is not None else 9.0
rmax = {v: raio_cabelo(v) for v in interior}
for _ in range(25):
    rmax = {v: min(rmax[v], sum(rmax.get(n, rmax[v]) for n in viz[v]) / len(viz[v])) for v in interior}
def limitar():
    for v in interior:
        dv = v.co - centro
        if dv.length > rmax[v]:
            v.co = centro + dv.normalized() * rmax[v]
limitar()
print('CUPULA topo z', round(max(v.co.z for v in interior), 4), 'limitados pelo cabelo',
      sum(1 for v in interior if (v.co - centro).length >= rmax[v] - 1e-5))

# Arredondamento: as paredes originais (laterais/nuca) sobem retas até ~5 cm do topo e a tampa ficava chata
# ("caixa"). Acima de Z0 (altura dos olhos; na testa só acima das sobrancelhas) parede + cúpula viram um quarto de
# elipse em cada direção: raio R da borda -> 0, altura Z0 -> TOPO_Z. Vértices da parede mantêm a distância até a
# superfície (orelha/relevo descem junto); a cúpula é recolocada pelo parâmetro ao longo do perfil.
interior_set = set(interior)
mats_pele = (MAT_PELE, MAT_ROSTO, mat_cranio)
def z0_em(theta):
    fr = math.sin(theta) * -1          # 1 = frente (-Y)
    t = min(max((fr - 0.3) / 0.5, 0.0), 1.0); t = t * t * (3 - 2 * t)
    return 1.62 + 0.045 * t
perfil = {}   # vértice -> (phi, R, H, direção radial) para a normal
movidos = 0
for v in bm_cabeca.verts:
    if not any(f.material_index in mats_pele for f in v.link_faces):
        continue
    dx, dy = v.co.x - eixo.x, v.co.y - eixo.y
    rho = math.hypot(dx, dy); theta = math.atan2(dy, dx)
    R, zb_ = borda_em(theta)[:2]
    z0 = z0_em(theta); H = TOPO_Z - z0
    rad = Vector((math.cos(theta), math.sin(theta), 0))
    if v.co.z <= z0 and v not in interior_set:
        perfil[v] = (0.0, R, H, rad, abs(rho - R))
        continue
    parede = max(zb_ - z0, 0.005)
    L = parede + R
    if v in interior_set:
        u = (parede + max(R - rho, 0.0)) / L
        phi = min(u, 1.0) * math.pi / 2
        r_novo = R * math.cos(phi)
        perfil[v] = (phi, R, H, rad, 0.0)
    else:
        u = min((v.co.z - z0) / L, parede / L)
        phi = u * math.pi / 2
        r_novo = rho - R * (1 - math.cos(phi))
        perfil[v] = (phi, R, H, rad, abs(rho - R))
    v.co = Vector((eixo.x + rad.x * r_novo, eixo.y + rad.y * r_novo, z0 + H * math.sin(phi)))
    movidos += 1
print('ARREDONDADO', movidos, 'vértices; topo', round(max(v.co.z for v in bm_cabeca.verts), 4))
bm_cabeca.normal_update()

# pesos: perto da borda = pesos da borda; para dentro, 100% head (3 cm)
kd = KDTree(len(topo))
for i, v in enumerate(topo):
    kd.insert(v.co, i)
kd.balance()
for v in interior:
    perto = kd.find_n(v.co, 3)
    dist = perto[0][2]
    t = min(dist / 0.03, 1.0)
    acc = {}
    wsum = sum(1 / max(p[2], 1e-5) for p in perto)
    for co_, i, dd in perto:
        for g, w in topo[i][dl].items():
            acc[g] = acc.get(g, 0) + w * (1 / max(dd, 1e-5)) / wsum
    hidx = next(i for i, n in gnames.items() if n == HEAD_BONE)
    final = {g: w * (1 - t) for g, w in acc.items()}
    final[hidx] = final.get(hidx, 0) + t
    for g, w in final.items():
        if w > 1e-4:
            v[dl][g] = w

# UV: pele lisa do rosto (testa) em toda a cúpula; cor de vértice R = escurecimento do couro (0 na borda -> 1 no topo)
rosto = [f for f in bm_cabeca.faces if f.material_index == MAT_ROSTO]
alvo = Vector((0, -0.085, 1.672))
f_testa = min(rosto, key=lambda f: (f.calc_center_median() - alvo).length)
for uvl in bm_cabeca.loops.layers.uv:
    uv_testa = f_testa.loops[0][uvl].uv.copy()
    for f in faces_cap:
        for l in f.loops:
            l[uvl].uv = uv_testa
cor = bm_cabeca.loops.layers.color.get('Couro') or bm_cabeca.loops.layers.color.new('Couro')
zb = max(v.co.z for v in borda)
for f in bm_cabeca.faces:
    for l in f.loops:
        s = 0.0
        if f in faces_cap:
            s = min(max((l.vert.co.z - zb + 0.01) / 0.03, 0.0), 1.0) if l.vert not in borda else 0.0
        l[cor] = (s, s, s, 1.0)

# ---------------------------------------------------------------- objetos e normais
def objeto(nome, bm, cap=None):
    bm.transform(Mi)
    me = bpy.data.meshes.new(nome); bm.to_mesh(me); bm.free()
    for m in src.data.materials:
        me.materials.append(m)
    if cap is not None:
        mc = bpy.data.materials.get(NOME_CRANIO) or bpy.data.materials.new(NOME_CRANIO)
        me.materials.append(mc)
    ob = bpy.data.objects.new(nome, me); bpy.context.scene.collection.objects.link(ob)
    ob.parent = src.parent; ob.matrix_parent_inverse = src.matrix_parent_inverse.copy(); ob.matrix_basis = src.matrix_basis.copy()
    for g in src.vertex_groups:
        ob.vertex_groups.new(name=g.name)
    mod = ob.modifiers.new('Indra', 'ARMATURE'); mod.object = rig
    return ob

# para as normais: distância de cada vértice da cúpula até a borda e o vértice da borda mais perto
bm_cabeca.verts.index_update()
dist_cap = {v.index: (kd.find(v.co)[2], topo[kd.find(v.co)[1]].index) for v in interior}
perfil_i = {v.index: d for v, d in perfil.items()}
ob_cab = objeto('SK_Indra_Head', bm_cabeca, cap=True)
ob_hair = objeto('SK_Hair_Indra', bm_cabelo)
ob_corpo = objeto('SK_Outfit_Indra', bm_corpo)
bpy.context.view_layer.update()

def copiar_normais(ob):
    dt = ob.modifiers.new('normais', 'DATA_TRANSFER'); dt.object = src
    dt.use_loop_data = True; dt.data_types_loops = {'CUSTOM_NORMAL'}; dt.loop_mapping = 'POLYINTERP_NEAREST'
    dt.use_object_transform = True
    ob.modifiers.move(len(ob.modifiers) - 1, 0)
    with bpy.context.temp_override(object=ob, active_object=ob, selected_objects=[ob]):
        bpy.ops.object.modifier_apply(modifier=dt.name)

for ob in (ob_hair, ob_corpo, ob_cab):
    copiar_normais(ob)

def limpar(ob):
    """Só os espaços de material usados (mesma ordem) e, na cabeça, só a cor de vértice Couro."""
    me = ob.data
    usados = sorted({p.material_index for p in me.polygons})
    novo = {old: i for i, old in enumerate(usados)}
    mats = [me.materials[i] for i in usados]
    idx = [novo[p.material_index] for p in me.polygons]
    me.materials.clear()
    for m in mats:
        me.materials.append(m)
    for p, i in zip(me.polygons, idx):
        p.material_index = i
    for ca in list(me.color_attributes):
        if ca.name != 'Couro':
            me.color_attributes.remove(ca)
    if 'Couro' in me.color_attributes:
        me.color_attributes.active_color = me.color_attributes['Couro']
        me.color_attributes.render_color_index = me.color_attributes.find('Couro')

# cúpula: normais da própria geometria; na borda, as do rosto (média das faces originais no vértice)
me = ob_cab.data
loop_n = [Vector(n.vector) for n in me.corner_normals]
mc_ = len(me.materials) - 1
geo_n = [Vector() for _ in me.vertices]
for p in me.polygons:
    if p.material_index == mc_:
        for vi in p.vertices:
            geo_n[vi] += Vector(p.normal) * p.area
mc = len(me.materials) - 1
borda_orig = {}
for p in me.polygons:
    if p.material_index != mc:
        for li in p.loop_indices:
            borda_orig.setdefault(me.loops[li].vertex_index, Vector()).__iadd__(loop_n[li])
for p in me.polygons:
    if p.material_index == mc:
        for li in p.loop_indices:
            vi = me.loops[li].vertex_index
            if vi in borda_orig:
                n = borda_orig[vi].normalized()
            else:
                # transição suave da normal da borda (rosto/parede) para a da cúpula nos primeiros 2,5 cm
                dd, vb = dist_cap.get(vi, (1.0, None))
                w = min(dd / 0.025, 1.0); w = w * w * (3 - 2 * w)
                nb_n = borda_orig[vb].normalized() if vb in borda_orig else geo_n[vi].normalized()
                n = nb_n.lerp(geo_n[vi].normalized(), w)
            loop_n[li] = n.normalized()
# Laterais/nuca/topo: as duas cascas do rip (máscara do rosto por cima do cilindro) têm normais diferentes e faziam
# uma faixa escura atrás da orelha. Ali a normal vira a do perfil arredondado (sombreamento liso de anime),
# misturada com a original na frente do rosto, embaixo (pescoço) e nas orelhas (longe da superfície da parede).
M3t = M.to_3x3().transposed()
def sstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0); return t * t * (3 - 2 * t)
for p in me.polygons:
    if p.material_index not in (MAT_PELE, MAT_ROSTO, mc):
        continue
    for li in p.loop_indices:
        vi = me.loops[li].vertex_index
        if vi not in perfil_i:
            continue
        phi, R, H, rad, fora = perfil_i[vi]
        pw = M @ me.vertices[vi].co
        # vértice que se moveu (phi > 0, inclusive na testa) sempre usa a normal do perfil
        w = max(sstep(-0.06, -0.02, pw.y), sstep(0.0, 0.12, phi)) * sstep(1.555, 1.60, pw.z) * (1 - sstep(0.006, 0.014, fora))
        if w <= 0:
            continue
        nw = (rad * (math.cos(phi) / R) + Vector((0, 0, math.sin(phi) / H))).normalized()
        loop_n[li] = loop_n[li].lerp((M3t @ nw).normalized(), w).normalized()
me.normals_split_custom_set(loop_n)
for ob in (ob_hair, ob_corpo, ob_cab):
    limpar(ob)

# anel do pescoço (padrão de corte dos trajes), em metros no mundo do Indra
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'anel_pescoco.json').write_text(json.dumps({'corte_z_m': CORTE_Z, 'pontos_m': anel_pts,
                                                   'nota': 'mundo do Indra_Referencia em repouso, metros, Z para cima'}, indent=1))
info = {'cupula_vertices': len(interior), 'cupula_faces': len(faces_cap), 'borda_topo': len(topo), 'anel': len(anel or []),
        'cabeca_verts': len(ob_cab.data.vertices), 'cabelo_verts': len(ob_hair.data.vertices), 'corpo_verts': len(ob_corpo.data.vertices)}
print('INFO', json.dumps(info))

if FOTOS:
    FOTOS.mkdir(parents=True, exist_ok=True)
    sc = bpy.context.scene; sc.render.engine = 'BLENDER_WORKBENCH'; sc.display.shading.color_type = 'MATERIAL'
    sc.render.resolution_x = sc.render.resolution_y = 420
    for m in ob_cab.data.materials:
        if m:
            m.diffuse_color = (0.85, 0.68, 0.58, 1) if m.name in ('M_Indra', 'M_Indra_4', NOME_CRANIO) else (0.5, 0.5, 0.55, 1)
    cd = bpy.data.cameras.new('c'); cd.type = 'ORTHO'; cam = bpy.data.objects.new('c', cd); sc.collection.objects.link(cam); sc.camera = cam
    zc = 1.60
    vistas = {'frente': ((0, -3, zc), (math.pi / 2, 0, 0)), 'lado': ((3, 0.02, zc), (math.pi / 2, 0, math.pi / 2)),
              'costas': ((0, 3, zc), (math.pi / 2, 0, math.pi)), 'cima': ((0, 0.02, 4), (0, 0, 0)),
              'tres4': ((2.2, -2.2, zc + 1.0), (math.radians(68), 0, math.radians(45)))}
    for com_cabelo in (False, True):
        ob_hair.hide_render = not com_cabelo; ob_corpo.hide_render = True; src.hide_render = True
        for nome, (loc, rot) in vistas.items():
            cd.ortho_scale = 0.36
            cam.location = loc; cam.rotation_euler = rot
            if nome == 'tres4':
                cam.location = Vector((0, 0.02, zc)) + Vector((0, 0, 1)).lerp(Vector((1, -1, 0.4)).normalized(), 1) * 3
                cam.rotation_euler = (Vector((0, 0.02, zc)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
            sc.render.filepath = str(FOTOS / f"{'cabelo' if com_cabelo else 'careca'}_{nome}.png")
            bpy.ops.render.render(write_still=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(FOTOS / 'indra_modular.blend'))

if not SEM_FBX:
    for ob in (ob_cab, ob_hair, ob_corpo):
        bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); ob.select_set(True)
        bpy.context.view_layer.objects.active = ob
        bpy.ops.export_scene.fbx(filepath=str(OUT / (ob.name + '.fbx')), use_selection=True, object_types={'ARMATURE', 'MESH'},
                                 add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False, apply_unit_scale=True,
                                 axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', colors_type='LINEAR', path_mode='AUTO')
        print('EXPORTADO', ob.name)
    (OUT / 'info.json').write_text(json.dumps(info, indent=1))
