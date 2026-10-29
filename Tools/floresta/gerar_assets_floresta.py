"""Assets estilizados da floresta do tutorial (25/10/2026), feitos por código no Blender (sem baixar nada, sem créditos).
Estilo da referência do Felipe (floresta toon): copas grandes e "fofas" feitas de bolhas, tronco grosso com base larga e
raízes, pedras de faces largas e lisas, grama alta em tufos, flores rosas/brancas, arbustos.
Cor por vertex color (o material da Unreal pinta): copa R = luz (fora/cima claro, dentro escuro); grama R = altura da
folha (0 base, 1 ponta, também é a máscara do vento); pedra R = topo; flor R = pétala, G = miolo.
Saída: Desktop/Hype World/Floresta/FBX/SM_*.fbx (unidade metro; origem no pé de cada objeto).
Rodar: blender -b --factory-startup --python Tools/floresta/gerar_assets_floresta.py
"""
import bpy, bmesh, math, os, random
from mathutils import Vector, noise, Matrix

SAIDA = "/Users/luisfelipexavier/Desktop/Hype World/Floresta/FBX"
os.makedirs(SAIDA, exist_ok=True)


def limpar():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def material(nome):
    m = bpy.data.materials.get(nome) or bpy.data.materials.new(nome)
    return m


def objeto(nome, bm, materiais, normais=None):
    me = bpy.data.meshes.new(nome)
    bm.to_mesh(me)
    bm.free()
    for m in materiais:
        me.materials.append(material(m))
    ob = bpy.data.objects.new(nome, me)
    bpy.context.scene.collection.objects.link(ob)
    if normais is not None:
        me.normals_split_custom_set_from_vertices(normais(me))
    return ob


def exportar(ob):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath="%s/%s.fbx" % (SAIDA, ob.name), use_selection=True, apply_scale_options="FBX_SCALE_ALL",
                             mesh_smooth_type="OFF", use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False,
                             axis_forward="-Y", axis_up="Z", colors_type="LINEAR")
    print("exportado", ob.name, len(ob.data.polygons), "faces")


def cor(bm):
    return bm.loops.layers.color.get("Col") or bm.loops.layers.color.new("Col")


# ------------------------------------------------------------------ peças --------------------------------------------------
def tubo(bm, base, topo, r0, r1, lados=10, aneis=6, curva=Vector((0, 0, 0)), flare=0.0, mat=0, col=None, valor=(0.5, 0.5, 0.5, 1)):
    """Tubo afinando de base (raio r0) até topo (r1), com uma curva no meio e alargando na base (flare)."""
    eixo = (topo - base)
    z = eixo.normalized()
    x = z.orthogonal().normalized()
    y = z.cross(x)
    aneis_v = []
    for i in range(aneis + 1):
        t = i / aneis
        c = base + eixo * t + curva * math.sin(math.pi * t)
        r = r1 + (r0 - r1) * (1 - t) + flare * (1 - t) ** 4
        anel = []
        for k in range(lados):
            a = 2 * math.pi * k / lados
            rr = r * (1 + 0.08 * noise.noise(Vector((math.cos(a) * 2, math.sin(a) * 2, t * 3)) + base))
            anel.append(bm.verts.new(c + (x * math.cos(a) + y * math.sin(a)) * rr))
        aneis_v.append(anel)
    for i in range(aneis):
        for k in range(lados):
            f = bm.faces.new((aneis_v[i][k], aneis_v[i][(k + 1) % lados], aneis_v[i + 1][(k + 1) % lados], aneis_v[i + 1][k]))
            f.material_index = mat
            if col:
                for lp in f.loops:
                    lp[col] = valor
    tampa = bm.faces.new(aneis_v[-1])
    tampa.material_index = mat
    return aneis_v


def bolha(bm, centro, raio, mat, col, centro_copa, seed, subdiv=2, achatar=1.0):
    """Bolha de folhas: icosfera deformada por ruído. Vertex color R = luz (de cima/fora claro, dentro escuro)."""
    res = bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=raio)
    vs = res["verts"]
    off = Vector((seed * 1.7, seed * 3.1, seed * 0.7))
    for v in vs:
        d = v.co.normalized()
        n = noise.noise(d * 1.6 + off) * 0.22 + noise.noise(d * 4.0 + off) * 0.07
        v.co = d * raio * (1 + n)
        v.co.z *= achatar
        v.co += centro
    faces = {f for v in vs for f in v.link_faces}
    for f in faces:
        f.material_index = mat
        for lp in f.loops:
            p = lp.vert.co
            fora = (p - centro_copa)
            fora_n = fora.normalized() if fora.length > 0 else Vector((0, 0, 1))
            luz = 0.5 + 0.5 * fora_n.dot(Vector((0.35, -0.25, 0.9)).normalized())
            lado = min(1.0, fora.length / max(raio * 2.2, 0.01))
            r = max(0.0, min(1.0, 0.15 + 0.6 * luz * (0.4 + 0.6 * lado) + 0.25 * max(0.0, fora_n.z)))
            lp[col] = (r, lado, 0, 1)
    return vs


def normais_copa(centro_copa, peso=0.65):
    """Normais da copa puxadas para fora do centro (sombreado "fofo", como na referência); o tronco fica com as próprias."""
    def f(me):
        out = []
        for v in me.vertices:
            n = Vector(v.normal)
            if me.materials and v.index in _verts_folha:
                d = (Vector(v.co) - centro_copa).normalized()
                n = (n * (1 - peso) + d * peso).normalized()
            out.append(n)
        return out
    return f


_verts_folha = set()


# ------------------------------------------------------------------ árvore -------------------------------------------------
def arvore(nome, seed, altura_tronco, raio_tronco, copa_raio, copa_altura, n_bolhas, raizes=6):
    random.seed(seed)
    bm = bmesh.new()
    col = cor(bm)
    base = Vector((0, 0, -0.3))
    topo = Vector((random.uniform(-0.4, 0.4), random.uniform(-0.4, 0.4), altura_tronco))
    tubo(bm, base, topo, raio_tronco * 1.25, raio_tronco * 0.6, lados=14, aneis=9, curva=Vector((random.uniform(-0.5, 0.5), random.uniform(-0.5, 0.5), 0)),
         flare=raio_tronco * 1.1, mat=0, col=col, valor=(0.5, 0, 0, 1))
    # raízes: saem da base para fora e para baixo (metade enterrada)
    for i in range(raizes):
        a = 2 * math.pi * (i + random.uniform(-0.25, 0.25)) / raizes
        d = Vector((math.cos(a), math.sin(a), 0))
        comp = raio_tronco * random.uniform(2.2, 3.2)
        tubo(bm, Vector((0, 0, raio_tronco * 0.9)) + d * raio_tronco * 0.5, d * (raio_tronco + comp) + Vector((0, 0, -0.35)),
             raio_tronco * 0.62, raio_tronco * 0.16, lados=8, aneis=4, curva=Vector((0, 0, raio_tronco * 0.35)), mat=0, col=col, valor=(0.4, 0, 0, 1))
    # galhos grossos até a copa
    centro_copa = topo + Vector((0, 0, copa_altura * 0.35))
    for i in range(4):
        a = 2 * math.pi * i / 4 + random.uniform(-0.4, 0.4)
        ponta = centro_copa + Vector((math.cos(a) * copa_raio * 0.55, math.sin(a) * copa_raio * 0.55, random.uniform(-0.5, 1.0)))
        tubo(bm, topo - Vector((0, 0, altura_tronco * 0.12)), ponta, raio_tronco * 0.42, raio_tronco * 0.12, lados=7, aneis=4,
             curva=Vector((0, 0, 0.6)), mat=0, col=col, valor=(0.45, 0, 0, 1))
    # copa: bolhas num domo (mais na casca do domo, para parecer cheia por fora)
    antes = len(bm.verts)
    for i in range(n_bolhas):
        if i == 0:
            c = centro_copa + Vector((0, 0, copa_altura * 0.1))
            r = copa_raio * 0.62
        else:
            a = random.uniform(0, 2 * math.pi)
            el = random.uniform(-0.15, 1.0)
            dist = random.uniform(0.55, 0.95)
            c = centro_copa + Vector((math.cos(a) * math.cos(el * 1.2) * copa_raio * dist,
                                      math.sin(a) * math.cos(el * 1.2) * copa_raio * dist,
                                      el * copa_altura * 0.55))
            r = copa_raio * random.uniform(0.32, 0.5)
        bolha(bm, c, r, 1, col, centro_copa, seed * 31 + i, subdiv=3, achatar=0.85)
    bm.verts.ensure_lookup_table()
    global _verts_folha
    _verts_folha = set(range(antes, len(bm.verts)))
    ob = objeto(nome, bm, ["M_Tronco", "M_Folhas"], normais_copa(centro_copa))
    exportar(ob)


# ------------------------------------------------------------------ pedra --------------------------------------------------
def pedra(nome, seed, tamanho, cortes=7, subdiv=3):
    """Pedra de faces largas e lisas: esfera com ruído suave, cortada por planos (cada corte vira uma face plana)."""
    random.seed(seed)
    bm = bmesh.new()
    col = cor(bm)
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    off = Vector((seed, seed * 2.3, seed * 0.4))
    for v in bm.verts:
        d = v.co.normalized()
        v.co = d * (1 + noise.noise(d * 1.3 + off) * 0.18)
    for _ in range(cortes):
        n = Vector((random.uniform(-1, 1), random.uniform(-1, 1), random.uniform(-0.4, 1))).normalized()
        dcorte = random.uniform(0.62, 0.85)
        for v in bm.verts:
            h = v.co.dot(n)
            if h > dcorte:
                v.co -= n * (h - dcorte)
    for v in bm.verts:   # base plana (fica um pouco enterrada)
        if v.co.z < -0.55:
            v.co.z = -0.55
        v.co = Vector((v.co.x * tamanho[0] * 0.5, v.co.y * tamanho[1] * 0.5, (v.co.z + 0.55) * tamanho[2] / 1.55))
    bm.normal_update()
    for e in bm.edges:   # arestas vivas entre as faces cortadas, o resto liso
        if len(e.link_faces) == 2 and e.link_faces[0].normal.angle(e.link_faces[1].normal) > math.radians(32):
            e.smooth = False
    for f in bm.faces:
        f.smooth = True
        f.material_index = 0
        for lp in f.loops:
            lp[col] = (max(0.0, f.normal.z) * 0.8 + 0.2 * noise.noise(lp.vert.co * 0.6 + off) + 0.1, 0, 0, 1)
    ob = objeto(nome, bm, ["M_Pedra"])
    exportar(ob)


# ------------------------------------------------------------------ grama / flor / arbusto ---------------------------------
def grama(nome, seed, folhas=12, altura=(0.45, 0.8)):
    random.seed(seed)
    bm = bmesh.new()
    col = cor(bm)
    for i in range(folhas):
        a = random.uniform(0, 2 * math.pi)
        r = random.uniform(0.0, 0.28)
        p0 = Vector((math.cos(a) * r, math.sin(a) * r, -0.03))
        h = random.uniform(*altura)
        dirb = Vector((math.cos(a), math.sin(a), 0)) * random.uniform(0.12, 0.3) + Vector((random.uniform(-0.08, 0.08), random.uniform(-0.08, 0.08), 0))
        lado = Vector((-math.sin(a + 1.2), math.cos(a + 1.2), 0))
        larg = random.uniform(0.035, 0.055)
        segs = 3
        g_cor = random.random()
        for verso in (False, True):   # frente e verso com vértices próprios (normal para cima nos dois: sem verso preto)
            pares = []
            for s in range(segs + 1):
                t = s / segs
                c = p0 + Vector((0, 0, h * t)) + dirb * (t * t)
                w = larg * (1 - t) ** 0.8
                if s == segs:
                    pares.append((bm.verts.new(c), None, t))
                else:
                    pares.append((bm.verts.new(c - lado * w), bm.verts.new(c + lado * w), t))
            for s in range(segs):
                a0, b0, t0 = pares[s]
                a1, b1, t1 = pares[s + 1]
                vs = (a0, b0, b1, a1) if b1 else (a0, b0, a1)
                f = bm.faces.new(tuple(reversed(vs)) if verso else vs)
                f.material_index = 0
                for lp in f.loops:
                    tt = (lp.vert.co.z + 0.03) / h
                    lp[col] = (max(0.0, min(1.0, tt)), g_cor, 0, 1)
    ob = objeto(nome, bm, ["M_Grama"], lambda me: [Vector((0, 0, 1)) for _ in me.vertices])   # normal para cima: grama lisa
    exportar(ob)


def flor(nome, mat, seed):
    random.seed(seed)
    bm = bmesh.new()
    col = cor(bm)
    h = random.uniform(0.3, 0.42)
    tubo(bm, Vector((0, 0, -0.02)), Vector((0.03, 0, h)), 0.008, 0.006, lados=4, aneis=2, mat=0, col=col, valor=(0, 0, 1, 1))
    topo = Vector((0.03, 0, h))
    for k in range(5):
        a = 2 * math.pi * k / 5
        d = Vector((math.cos(a), math.sin(a), 0.15))
        l = Vector((-math.sin(a), math.cos(a), 0))
        for verso in (False, True):   # pétala com frente e verso
            vs = [bm.verts.new(topo), bm.verts.new(topo + d * 0.05 + l * 0.03), bm.verts.new(topo + d * 0.085 + Vector((0, 0, 0.012))), bm.verts.new(topo + d * 0.05 - l * 0.03)]
            f = bm.faces.new(list(reversed(vs)) if verso else vs)
            for lp in f.loops:
                lp[col] = (1, 0, 0, 1)
    res = bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.018)
    for v in res["verts"]:
        v.co += topo + Vector((0, 0, 0.01))
    for f in {f for v in res["verts"] for f in v.link_faces}:
        for lp in f.loops:
            lp[col] = (0, 1, 0, 1)
    ob = objeto(nome, bm, [mat], lambda me: [Vector((0, 0, 1)) for _ in me.vertices])
    exportar(ob)


def arbusto(nome, seed):
    random.seed(seed)
    bm = bmesh.new()
    col = cor(bm)
    centro = Vector((0, 0, 0.55))
    antes = len(bm.verts)
    for i in range(6):
        a = random.uniform(0, 2 * math.pi)
        c = centro + Vector((math.cos(a) * 0.45, math.sin(a) * 0.45, random.uniform(-0.15, 0.35))) * (0 if i == 0 else 1)
        bolha(bm, c, random.uniform(0.42, 0.6) if i else 0.62, 0, col, centro, seed * 7 + i, subdiv=3, achatar=0.85)
    global _verts_folha
    _verts_folha = set(range(antes, len(bm.verts)))
    ob = objeto(nome, bm, ["M_Folhas"], normais_copa(centro))
    exportar(ob)


limpar()
arvore("SM_Arvore_Grande_A", 1, altura_tronco=8.5, raio_tronco=0.75, copa_raio=5.5, copa_altura=5.5, n_bolhas=14)
arvore("SM_Arvore_Grande_B", 2, altura_tronco=10.0, raio_tronco=0.85, copa_raio=6.0, copa_altura=6.0, n_bolhas=15)
arvore("SM_Arvore_Media_A", 3, altura_tronco=5.5, raio_tronco=0.45, copa_raio=3.6, copa_altura=3.8, n_bolhas=10, raizes=5)
pedra("SM_Pedra_Grande_A", 11, (3.6, 3.2, 5.0))
pedra("SM_Pedra_Grande_B", 12, (5.2, 3.6, 3.6))
pedra("SM_Pedra_Grande_C", 13, (4.2, 4.0, 4.2))
pedra("SM_Pedra_Pequena_A", 14, (0.7, 0.6, 0.45), subdiv=2)
pedra("SM_Pedra_Pequena_B", 15, (0.5, 0.45, 0.4), subdiv=2)
grama("SM_Grama_Tufo_A", 21)
grama("SM_Grama_Tufo_B", 22, folhas=10, altura=(0.35, 0.6))
flor("SM_Flor_Rosa", "M_Flor_Rosa", 31)
flor("SM_Flor_Branca", "M_Flor_Branca", 32)
arbusto("SM_Arbusto_A", 41)
print("FIM", sorted(os.listdir(SAIDA)))
