# Piloto do traje modular (01/11/2026): corpo do Itachi Anbu (Docs/ItachiAnbu, uniforme do pescoço para baixo) no rig do
# Indra (cabeça SK_Indra_Head, sem mexer em osso nem bind pose). O corpo vai de pose T para a pose A do Indra.
# - Encaixe: cada osso do Itachi vai para a junta equivalente do Indra. Só o comprimento do osso muda; a grossura usa uma
#   escala única (altura), então a armadura não distorce e os ombros ficam estreitos porque os braços andam para dentro.
# - Pesos: os mesmos do Itachi, renomeados para os ossos do Indra; braço e perna do Indra deformam pelos ossos
#   auxiliares (arm-bone, foot-bone), então o peso é repartido entre eles pela posição ao longo do osso.
# - Pescoço: corta logo abaixo do anel e costura uma faixa até os vértices do anel da cabeça, com a mesma posição,
#   os mesmos pesos e as normais da cabeça: a emenda não abre nas animações.
# - Pele: máscara por cor das texturas (T_*_SkinMask) para o parâmetro SkinTone do material.
# Uso: blender -b --factory-startup --python Tools/customization/traje_itachi_piloto.py -- [--personagem KakashiAnbu] [--fotos <pasta>] [--sem-fbx]
# Sem --personagem = Itachi Anbu (o piloto). Vale para os GLBs no rig padrão da Unreal, em Docs/<Personagem>/SK_<Personagem>.glb.
import bpy, bmesh, json, math, struct, sys, tempfile
import numpy as np
from pathlib import Path
from mathutils import Vector, Matrix, kdtree

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'SourceAssets/Customization/IndraModular'
args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
PERSONAGEM = args[args.index('--personagem') + 1] if '--personagem' in args else 'ItachiAnbu'
GLB = ROOT / f'Docs/{PERSONAGEM}/SK_{PERSONAGEM}.glb'
ORIGEM = BASE / 'Origem' / PERSONAGEM
# Personagens que só existem na Unreal (exportados por Tools/editor/exportar_origem_traje): materiais do corpo e texturas
ORIGENS_FBX = {
    'Shisui': {'materiais': ('MI_Shisui_Corpo',), 'texturas': ('T_Shisui_Corpo',)},
    'Sasuke': {'materiais': ('M_Sasuke_Blusa', 'M_Sasuke_Calca', 'M_Sasuke_Sapato'), 'texturas': ('T_Sasuke_Blusa', 'T_Sasuke_Calca', 'T_Sasuke_Sapato')},
}
FBX_ORIGEM = ORIGENS_FBX.get(PERSONAGEM)
# Rig do Sasuke -> nomes do rig padrão da Unreal, que o resto do script usa; ossos fora do mapa seguem o pai (arma, mochila)
def nomes_sasuke():
    m = {'bone_Hips': 'pelvis', 'bone_Spine': 'spine_01', 'bone_Spine1': 'spine_03', 'bone_Neck': 'neck_01', 'bone_Head': 'head', 'bone_Head_end': 'head_end'}
    for lado, s in (('Left', 'l'), ('Right', 'r')):
        m.update({f'bone_{lado}Clav': f'clavicle_{s}', f'bone_{lado}Arm': f'upperarm_{s}', f'bone_{lado}ForeArm': f'lowerarm_{s}', f'bone_{lado}Hand': f'hand_{s}',
                  f'bone_{lado}LegUpper': f'thigh_{s}', f'bone_{lado}Leg': f'calf_{s}', f'bone_{lado}Ankle': f'foot_{s}', f'bone_{lado}Toe': f'ball_{s}', f'bone_{lado}Toe_end': f'ball_end_{s}'})
        for k, dedo in enumerate(('thumb', 'index', 'middle', 'ring', 'pinky')):
            m.update({f'bone_{lado}_Finger{k}1' if k else f'bone_{lado}_Finger01': f'{dedo}_01_{s}', f'bone_{lado}_Finger{k}2' if k else f'bone_{lado}_Finger02': f'{dedo}_02_{s}',
                      f'bone_{lado}_Finger{k}2_end' if k else f'bone_{lado}_Finger02_end': f'{dedo}_03_{s}'})
    return m
RENOMEAR = nomes_sasuke() if PERSONAGEM == 'Sasuke' else {}
OUT = BASE / PERSONAGEM
FOTOS = Path(args[args.index('--fotos') + 1]) if '--fotos' in args else None
SEM_FBX = '--sem-fbx' in args
NOME = f'SK_Outfit_{PERSONAGEM}'
MATERIAIS_CORPO = ('kk2_tex01', 'kk2_tex02')   # os outros são rosto, olhos, faixa e cabelo
# m: altura da faixa costurada entre o corte do corpo e o anel. A gola do Itachi já termina em 1,4775 m; quem tem a gola
# inteira corta nessa mesma altura (cortar mais alto deixa um dente na gola de costas).
FAIXA = 0.012 if PERSONAGEM == 'ItachiAnbu' else 0.0225
OUT.mkdir(parents=True, exist_ok=True)

# ---------------------------------------------------------------- Indra (cabeça + rig)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(BASE / 'SK_Indra_Head.fbx'))
rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
cabeca = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
rig.data.pose_position = 'REST'
bpy.context.view_layer.update()
RM = rig.matrix_world.copy()
HM = cabeca.matrix_world.copy(); HMi = HM.inverted()
anel_json = json.loads((BASE / 'anel_pescoco.json').read_text())
CORTE_Z = anel_json['corte_z_m']

def osso_indra(frag):
    p = '9ind00t0-' + frag + '_'
    return next(b for b in rig.data.bones if b.name.startswith(p))
def J(frag):
    return np.array(RM @ osso_indra(frag).head_local)

def anel_da_cabeca():
    me = cabeca.data
    nv = {}
    for li, l in enumerate(me.loops):
        nv.setdefault(l.vertex_index, Vector()).__iadd__(Vector(me.corner_normals[li].vector))
    bm = bmesh.new(); bm.from_mesh(me); bm.verts.ensure_lookup_table()
    M3 = HM.to_3x3().inverted().transposed()
    gn = {g.index: g.name for g in cabeca.vertex_groups}
    dl = bm.verts.layers.deform.active
    out = []
    for v in bm.verts:
        co = HM @ v.co
        if any(e.is_boundary for e in v.link_edges) and min((co - Vector(q)).length for q in anel_json['pontos_m']) < 1e-4:
            out.append({'co': np.array(co), 'pesos': {gn[g]: w for g, w in v[dl].items()},
                        'n': (M3 @ nv[v.index]).normalized()})
    bm.free()
    return out
ANEL = anel_da_cabeca()
assert len(ANEL) == len(anel_json['pontos_m']), len(ANEL)

# ---------------------------------------------------------------- Itachi (cópia do GLB com o sampler que falta)
antes = set(bpy.data.objects)
if FBX_ORIGEM:
    bpy.ops.import_scene.fbx(filepath=str(ORIGEM / (PERSONAGEM + '.fbx')))
    MATERIAIS_CORPO = FBX_ORIGEM['materiais']
else:
    b = GLB.read_bytes()
    L = struct.unpack('<I', b[12:16])[0]; g = json.loads(b[20:20 + L]); resto = b[20 + L:]
    g.setdefault('samplers', [{}])
    for t in g.get('textures', []):
        if t.get('sampler', 0) >= len(g['samplers']):
            t['sampler'] = 0
    j = json.dumps(g).encode(); j += b' ' * ((4 - len(j) % 4) % 4)
    tmp = Path(tempfile.mkdtemp()) / 'itachi.glb'
    tmp.write_bytes(struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(j) + len(resto)) + struct.pack('<I', len(j)) + b'JSON' + j + resto)
    bpy.ops.import_scene.gltf(filepath=str(tmp))
novos = set(bpy.data.objects) - antes
arm_i = next(o for o in novos if o.type == 'ARMATURE')
src = next(o for o in novos if o.type == 'MESH' and any(m and m.name.startswith(MATERIAIS_CORPO) for m in o.data.materials))
SLOTS_CORPO = [i for n in MATERIAIS_CORPO for i, m in enumerate(src.data.materials) if m and m.name.startswith(n)]
print('SLOTS corpo', SLOTS_CORPO, [m.name for m in src.data.materials])
arm_i.data.pose_position = 'REST'
for velho, novo in RENOMEAR.items():
    if velho in arm_i.data.bones:
        arm_i.data.bones[velho].name = novo   # o Blender renomeia junto os grupos de vértices da malha ligada
bpy.context.view_layer.update()
AM = arm_i.matrix_world.copy()
H0 = {b.name: np.array(AM @ b.head_local) for b in arm_i.data.bones}
print('ITACHI pe', H0['foot_l'], 'dedo', H0['ball_l'], 'ombro', H0['upperarm_l'], 'cabeca', H0['head'])

# ---------------------------------------------------------------- juntas de destino
ESCALA = float((J('head') - J('pelvis'))[2] / (H0['head'] - H0['pelvis'])[2])
# os dois Anbu têm a altura medida no GLB inteiro (o Itachi sem o cabelo daria 1,91 e mudaria o traje aprovado)
ALTURA_ORIGEM = round(max((src.matrix_world @ v.co).z for v in src.data.vertices), 2) if FBX_ORIGEM else 1.93
ESCALA = min(ESCALA, 1.8 / ALTURA_ORIGEM)            # grossura: altura do Indra / altura do personagem
T = {'root': np.zeros(3), 'pelvis': J('pelvis')}
# coluna: mesma fração do comprimento, ao longo pelvis -> spine -> spine1 -> neck do Indra
cad = [n for n in ('pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01') if n in H0]
comp = np.cumsum([0] + [np.linalg.norm(H0[cad[i + 1]] - H0[cad[i]]) for i in range(len(cad) - 1)]); comp /= comp[-1]
linha = [J('pelvis'), J('spine'), J('spine1'), J('neck')]
seg = np.cumsum([0] + [np.linalg.norm(linha[i + 1] - linha[i]) for i in range(3)]); seg /= seg[-1]
for n, t in zip(cad, comp):
    T[n] = np.array([np.interp(t, seg, [p[k] for p in linha]) for k in range(3)])
T['neck_02'] = (J('neck') + J('head')) / 2
T['head'] = J('head')
FALANGES = {'index': '1', 'middle': '2', 'ring': '3', 'pinky': '4'}
for s, i in (('l', 'l'), ('r', 'r')):
    T[f'clavicle_{s}'] = J(f'{i}-clavicle'); T[f'upperarm_{s}'] = J(f'{i}-upperarm')
    T[f'lowerarm_{s}'] = J(f'{i}-forearm'); T[f'hand_{s}'] = J(f'{i}-hand')
    T[f'thigh_{s}'] = J(f'{i}-thigh'); T[f'calf_{s}'] = J(f'{i}-calf'); T[f'foot_{s}'] = J(f'{i}-foot')
    T[f'ball_{s}'] = J(f'{i}-toe0')
    T[f'thumb_01_{s}'] = J(f'{i}-finger0'); T[f'thumb_02_{s}'] = J(f'{i}-finger01'); T[f'thumb_03_{s}'] = J(f'{i}-finger02')
    for dedo, k in FALANGES.items():
        T[f'{dedo}_01_{s}'] = J(f'{i}-finger{k}'); T[f'{dedo}_02_{s}'] = J(f'{i}-finger{k}1'); T[f'{dedo}_03_{s}'] = J(f'{i}-finger{k}2')
        if f'{dedo}_metacarpal_{s}' in H0:
            r = np.linalg.norm(H0[f'{dedo}_metacarpal_{s}'] - H0[f'hand_{s}']) / np.linalg.norm(H0[f'{dedo}_01_{s}'] - H0[f'hand_{s}'])
            T[f'{dedo}_metacarpal_{s}'] = T[f'hand_{s}'] + (T[f'{dedo}_01_{s}'] - T[f'hand_{s}']) * r

FILHO = {a_: c_ for a_, c_ in zip(cad[1:], cad[2:])}
if 'neck_02' in H0:
    FILHO.update({'neck_01': 'neck_02', 'neck_02': 'head'})
else:
    FILHO['neck_01'] = 'head'
for s in 'lr':
    FILHO.update({f'clavicle_{s}': f'upperarm_{s}', f'upperarm_{s}': f'lowerarm_{s}', f'lowerarm_{s}': f'hand_{s}',
                  f'thigh_{s}': f'calf_{s}', f'calf_{s}': f'foot_{s}', f'foot_{s}': f'ball_{s}',
                  f'thumb_01_{s}': f'thumb_02_{s}', f'thumb_02_{s}': f'thumb_03_{s}'})
    for dedo in FALANGES:
        FILHO.update({f'{dedo}_metacarpal_{s}': f'{dedo}_01_{s}', f'{dedo}_01_{s}': f'{dedo}_02_{s}', f'{dedo}_02_{s}': f'{dedo}_03_{s}'})
FILHO = {a_: c_ for a_, c_ in FILHO.items() if a_ in H0 and c_ in H0}

def rot_entre(a, b_):
    return np.array(Vector(a).normalized().rotation_difference(Vector(b_).normalized()).to_matrix())

def rot_mao(s):
    """Mão: alinha também o giro (eixo dos dedos + eixo indicador -> mindinho), para os dedos não torcerem."""
    def base(P):
        x = P[f'middle_01_{s}'] - P[f'hand_{s}']; x = x / np.linalg.norm(x)
        y = P[f'pinky_01_{s}'] - P[f'index_01_{s}']; y = y - x * np.dot(x, y); y = y / np.linalg.norm(y)
        return np.column_stack((x, y, np.cross(x, y)))
    return base(T) @ base(H0).T

# matriz de cada osso: p -> T + Q * A * (p - H0)
Q, A = {}, {}
ordem = [b.name for b in arm_i.data.bones]  # pais antes dos filhos
for b in arm_i.data.bones:
    n = b.name
    pai = b.parent.name if b.parent else None
    if n in FILHO:
        d0 = H0[FILHO[n]] - H0[n]; d1 = T[FILHO[n]] - T[n]
        Q[n] = rot_entre(d0, d1)
        u = d0 / np.linalg.norm(d0)
        A[n] = ESCALA * np.eye(3) + (np.linalg.norm(d1) / np.linalg.norm(d0) - ESCALA) * np.outer(u, u)
    elif n.startswith('hand_'):
        Q[n] = rot_mao(n[-1]); A[n] = ESCALA * np.eye(3)
    else:
        Q[n] = Q[pai] if pai in Q else np.eye(3); A[n] = ESCALA * np.eye(3)
    if n not in T:   # ponta (dedo, bola do pé, cabeça): segue o pai
        T[n] = T[pai] + Q[pai] @ A[pai] @ (H0[n] - H0[pai]) if pai else np.zeros(3)

# ---------------------------------------------------------------- pesos: osso do Itachi -> ossos do Indra
def t_ao_longo(frag, a, c):
    va, vc = J(a), J(c)
    return float(np.dot(J(frag) - va, vc - va) / np.dot(vc - va, vc - va))
MAPA, REPARTE = {'root': 'pelvis', 'pelvis': 'pelvis', 'spine_01': 'spine', 'spine_02': 'spine', 'spine_03': 'spine1',
                 'spine_04': 'spine1', 'spine_05': 'spine1', 'neck_01': 'neck', 'neck_02': 'neck', 'head': 'head1',
                 'head_end': 'head1', 'Boca': 'head1'}, {}
for s in 'lr':
    MAPA.update({f'clavicle_{s}': f'{s}-clavicle', f'hand_{s}': f'{s}-hand', f'foot_{s}': f'{s}-foot',
                 f'ball_{s}': f'{s}-toe0', f'ball_end_{s}': f'{s}-toe0', f'thumb_01_{s}': f'{s}-finger0',
                 f'thumb_02_{s}': f'{s}-finger01', f'thumb_03_{s}': f'{s}-finger02', f'thumb_04_{s}': f'{s}-finger02'})
    for dedo, k in FALANGES.items():
        MAPA.update({f'{dedo}_metacarpal_{s}': f'{s}-hand', f'{dedo}_01_{s}': f'{s}-finger{k}', f'{dedo}_02_{s}': f'{s}-finger{k}1',
                     f'{dedo}_03_{s}': f'{s}-finger{k}2', f'{dedo}_04_{s}': f'{s}-finger{k}2'})
    REPARTE[f'upperarm_{s}'] = (f'{s}-upperarm', f'{s}-forearm', [f'{s}-arm-bone01', f'{s}-arm-bone02', f'{s}-arm-bone03'])
    REPARTE[f'lowerarm_{s}'] = (f'{s}-forearm', f'{s}-hand', [f'{s}-arm-bone04', f'{s}-arm-bone05', f'{s}-arm-bone06'])
    REPARTE[f'thigh_{s}'] = (f'{s}-thigh', f'{s}-calf', [f'{s}-thigh', f'{s}-foot-bone01', f'{s}-foot-bone02'])
    REPARTE[f'calf_{s}'] = (f'{s}-calf', f'{s}-foot', [f'{s}-foot-bone03', f'{s}-foot-bone04', f'{s}-foot-bone05'])
nome_i = lambda frag: osso_indra(frag).name
GRUPOS_REP = {k: (J(a), J(c), [(nome_i(f), t_ao_longo(f, a, c)) for f in fs]) for k, (a, c, fs) in REPARTE.items()}

PAI_I = {b.name: b.parent.name if b.parent else None for b in arm_i.data.bones}
def conhecido(n):
    """Osso sem equivalente (twist, arma, mochila): vale o primeiro pai que tem."""
    while n not in GRUPOS_REP and n not in MAPA and PAI_I.get(n):
        n = PAI_I[n]
    return n if n in GRUPOS_REP or n in MAPA else 'pelvis'
def pesos_indra(ossos_w, p):
    out = {}
    for n0, w in ossos_w.items():
        n = conhecido(n0)
        if n in GRUPOS_REP:
            va, vc, lista = GRUPOS_REP[n]
            t = float(np.dot(p - va, vc - va) / np.dot(vc - va, vc - va))
            if t <= lista[0][1]:
                out[lista[0][0]] = out.get(lista[0][0], 0) + w
            elif t >= lista[-1][1]:
                out[lista[-1][0]] = out.get(lista[-1][0], 0) + w
            else:
                for (b0, t0), (b1, t1) in zip(lista, lista[1:]):
                    if t0 <= t <= t1:
                        f = (t - t0) / (t1 - t0)
                        out[b0] = out.get(b0, 0) + w * (1 - f); out[b1] = out.get(b1, 0) + w * f
                        break
        else:
            alvo = nome_i(MAPA[n])
            out[alvo] = out.get(alvo, 0) + w
    soma = sum(out.values()) or 1.0
    return {k: v / soma for k, v in out.items()}

# ---------------------------------------------------------------- deforma o corpo (posição e normal)
me_s = src.data
SM = src.matrix_world.copy()
gn_s = {g.index: g.name for g in src.vertex_groups}
fit_co, fit_w = [], []
N3 = {n: np.linalg.inv(Q[n] @ A[n]).T for n in ordem}
for v in me_s.vertices:
    p = np.array(SM @ v.co)
    w = {gn_s[g.group]: g.weight for g in v.groups if g.weight > 1e-4}
    tot = sum(w.values()) or 1.0
    np_ = sum((T[n] + Q[n] @ A[n] @ (p - H0[n])) * (x / tot) for n, x in w.items()) if w else p
    fit_co.append(np_); fit_w.append(w)
SN = SM.to_3x3().inverted().transposed()
loop_n = []
for li, l in enumerate(me_s.loops):
    n0 = np.array((SN @ Vector(me_s.corner_normals[li].vector)).normalized())
    w = fit_w[l.vertex_index]; tot = sum(w.values()) or 1.0
    n1 = sum((N3[n] @ n0) * (x / tot) for n, x in w.items()) if w else n0
    loop_n.append(tuple(n1 / (np.linalg.norm(n1) or 1)))

# malha de referência já encaixada (mundo do Indra), só para copiar as normais depois
ref_me = bpy.data.meshes.new('ref')
ref_me.from_pydata([tuple(c) for c in fit_co], [], [tuple(p.vertices) for p in me_s.polygons])
for p, q in zip(ref_me.polygons, me_s.polygons):
    p.use_smooth = True
ref_n = [None] * len(ref_me.loops)
for p, q in zip(ref_me.polygons, me_s.polygons):
    for a_, b_ in zip(p.loop_indices, q.loop_indices):
        ref_n[a_] = loop_n[b_]
ref_me.normals_split_custom_set(ref_n)
ref_ob = bpy.data.objects.new('ref', ref_me); bpy.context.scene.collection.objects.link(ref_ob)

# ---------------------------------------------------------------- monta o traje no mundo do Indra
bm = bmesh.new(); bm.from_mesh(me_s); bm.verts.ensure_lookup_table()
dl = bm.verts.layers.deform.active
uvl = bm.loops.layers.uv.active
for v in bm.verts:
    v.co = Vector(fit_co[v.index])
    for g_ in list(v[dl].keys()):
        del v[dl][g_]
peso_v = {v: pesos_indra(fit_w[v.index], fit_co[v.index]) for v in bm.verts}
bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index not in SLOTS_CORPO], context='FACES')
if PERSONAGEM == 'Shisui':
    # cabeça na mesma malha do corpo: dentes, língua e rosto ficam abaixo do corte depois do encaixe; saem pelo peso no osso da cabeça
    def so_na_cabeca(v):
        w = fit_w[v.index]
        return sum(x for n, x in w.items() if n in ('head', 'head_end', 'Boca')) >= 0.5 * (sum(w.values()) or 1.0)
    na_cabeca = {v for v in bm.verts if so_na_cabeca(v)}
    faces_cabeca = [f for f in bm.faces if all(v in na_cabeca for v in f.verts)]
    print('CABECA por peso', len(faces_cabeca), 'faces')
    bmesh.ops.delete(bm, geom=faces_cabeca, context='FACES')
bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
for f in bm.faces:
    f.material_index = SLOTS_CORPO.index(f.material_index)
# o rip tem triângulos soltos: solda os coincidentes (o UV fica por canto, não se perde)
bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=2e-5)
peso_v = {v: w for v, w in peso_v.items() if v.is_valid}
bm.verts.ensure_lookup_table(); bm.normal_update()

# ilha do pescoço: a que passa pelo centro do anel acima do corte
anel_xy = np.array([a['co'][:2] for a in ANEL]); centro_xy = anel_xy.mean(0); raio_anel = float(np.linalg.norm(anel_xy - centro_xy, axis=1).max())
visto, ilhas = set(), []
for v0 in bm.verts:
    if v0 in visto:
        continue
    vis, pil = {v0}, [v0]
    while pil:
        for e in pil.pop().link_edges:
            for x in e.verts:
                if x not in vis:
                    vis.add(x); pil.append(x)
    visto |= vis; ilhas.append(vis)
Z_FIM = CORTE_Z - FAIXA
# cabeça do próprio personagem (cabelo, rosto, máscara em pedaços soltos): ilha toda acima do corte e perto do pescoço sai
def so_cabeca(vs):
    return all(v.co.z > Z_FIM and np.linalg.norm(np.array(v.co[:2]) - centro_xy) < 0.35 for v in vs)
ilhas_cabeca = [i for i in ilhas if so_cabeca(i)]
print('ILHAS só de cabeça removidas', len(ilhas_cabeca), 'vértices', sum(len(i) for i in ilhas_cabeca))
ilhas = [i for i in ilhas if i not in ilhas_cabeca]
bmesh.ops.delete(bm, geom=[v for i in ilhas_cabeca for v in i], context='VERTS')
peso_v = {v: w for v, w in peso_v.items() if v.is_valid}
def tem_pescoco(vs):
    return sum(1 for v in vs if v.co.z > Z_FIM and np.linalg.norm(np.array(v.co[:2]) - centro_xy) < raio_anel * 2.2)
ilhas_pescoco = [i for i in ilhas if tem_pescoco(i)]
print('ILHAS', len(ilhas), 'no pescoço', [(len(i), tem_pescoco(i)) for i in ilhas_pescoco])
geom = []
for i in ilhas_pescoco:
    fs = {f for v in i for f in v.link_faces}
    es = {e for v in i for e in v.link_edges}
    geom += list(i) + list(es) + list(fs)
bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-6, plane_co=(0, 0, Z_FIM), plane_no=(0, 0, 1), clear_outer=True, clear_inner=False)
bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
bm.verts.ensure_lookup_table(); bm.normal_update()
# vértices novos do corte: peso do vizinho mais próximo que tinha peso
for v in bm.verts:
    if v not in peso_v:
        viz = [e.other_vert(v) for e in v.link_edges if e.other_vert(v) in peso_v]
        if viz:
            a_ = min(viz, key=lambda o: (o.co - v.co).length)
            peso_v[v] = dict(peso_v[a_])
for _ in range(3):
    for v in bm.verts:
        if v not in peso_v:
            viz = [e.other_vert(v) for e in v.link_edges if e.other_vert(v) in peso_v]
            if viz:
                peso_v[v] = dict(peso_v[viz[0]])

# borda de cima do pescoço: o corte acima ou, se a gola já termina antes do plano, a própria abertura da gola
adj = {}
for e in bm.edges:
    if e.is_boundary and all(v.co.z > 1.38 for v in e.verts):
        a_, b_ = e.verts
        adj.setdefault(a_, []).append(b_); adj.setdefault(b_, []).append(a_)
lacos, vistos = [], set()
for v0 in adj:
    if v0 in vistos or len(adj[v0]) != 2:
        continue
    l_, prev, cur = [v0], None, v0; vistos.add(v0)
    while True:
        nx = [n for n in adj[cur] if n is not prev and n not in vistos]
        if not nx:
            break
        prev, cur = cur, nx[0]; l_.append(cur); vistos.add(cur)
    if len(l_) > 4:
        lacos.append(l_)
def raio_medio(l_):
    return float(np.mean([np.linalg.norm(np.array(v.co[:2]) - centro_xy) for v in l_]))
def z_medio(l_):
    return float(np.mean([v.co.z for v in l_]))
def rodeia(l_):
    """O laço dá a volta no pescoço (ângulos em todos os quadrantes), não é um buraco lateral."""
    return len({int(math.atan2(v.co.y - centro_xy[1], v.co.x - centro_xy[0]) // (math.pi / 2)) for v in l_}) == 4
print('LACOS gola', [(len(l_), round(raio_medio(l_), 3), round(z_medio(l_), 3), rodeia(l_)) for l_ in lacos], 'raio anel', round(raio_anel, 3))
candidatos = [l_ for l_ in lacos if rodeia(l_) and raio_medio(l_) < 0.12]
topo = max(z_medio(l_) for l_ in candidatos)
no_corte = [l_ for l_ in candidatos if z_medio(l_) > topo - 0.005]
laco = min(no_corte, key=lambda l_: abs(raio_medio(l_) - raio_anel))   # a gola é a borda do tamanho do anel
for l_ in no_corte:
    if l_ is not laco and raio_medio(l_) < raio_medio(laco):   # por dentro dela é resto da cabeça; por fora é gola alta, fica
        vis, pil = set(l_), list(l_)
        while pil:
            for e in pil.pop().link_edges:
                for x in e.verts:
                    if x not in vis:
                        vis.add(x); pil.append(x)
        assert not vis & set(laco)
        print('RESTO da cabeça removido', len(vis), 'vértices')
        bmesh.ops.delete(bm, geom=list(vis), context='VERTS')
peso_v = {v: w for v, w in peso_v.items() if v.is_valid}
# sobra da máscara do Kakashi: plaquinha embaixo do queixo, virada para baixo, solta à frente do pescoço
plaq = [] if FBX_ORIGEM else [f for f in bm.faces if f.material_index == 0 and f.calc_center_median().z > 1.45 and f.calc_center_median().y < 0.0
        and abs(f.calc_center_median().x) < 0.03]
print('PLAQUINHA removida', len(plaq), 'faces')
bmesh.ops.delete(bm, geom=plaq, context='FACES')
bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
peso_v = {v: w for v, w in peso_v.items() if v.is_valid}

# faixa: costura o laço do corpo aos vértices do anel, andando pelos dois em ordem de ângulo
ang = lambda xy: math.atan2(xy[1] - centro_xy[1], xy[0] - centro_xy[0]) % (2 * math.pi)
baixo = sorted(laco, key=lambda v: ang(v.co[:2]))
cima_d = sorted(ANEL, key=lambda a: ang(a['co'][:2]))
cima = [bm.verts.new(tuple(a['co'])) for a in cima_d]
for v, a in zip(cima, cima_d):
    peso_v[v] = dict(a['pesos'])
bm.verts.index_update()
faces_faixa = []
nb, nc = len(baixo), len(cima)
ib = ic = 0
ang_b = [ang(v.co[:2]) for v in baixo]; ang_c = [ang(v.co[:2]) for v in cima]
while ib < nb or ic < nc:
    pb = ang_b[ib % nb] + (2 * math.pi if ib >= nb else 0); pb1 = ang_b[(ib + 1) % nb] + (2 * math.pi if ib + 1 >= nb else 0)
    pc = ang_c[ic % nc] + (2 * math.pi if ic >= nc else 0); pc1 = ang_c[(ic + 1) % nc] + (2 * math.pi if ic + 1 >= nc else 0)
    if ic >= nc or (ib < nb and pb1 - pc <= pc1 - pb):
        tri = (baixo[ib % nb], baixo[(ib + 1) % nb], cima[ic % nc]); ib += 1
    else:
        tri = (baixo[ib % nb], cima[(ic + 1) % nc], cima[ic % nc]); ic += 1
    try:
        faces_faixa.append(bm.faces.new(tri))
    except ValueError:
        pass
bm.normal_update()
fora = sum(f.normal.dot(f.calc_center_median() - Vector((centro_xy[0], centro_xy[1], CORTE_Z))) for f in faces_faixa)
if fora < 0:
    for f in faces_faixa:
        f.normal_flip()
# material e UV da faixa = os da gola logo abaixo
for f in faces_faixa:
    v_b = next((v for v in f.verts if v in set(baixo)), None)
    vz = next(lf for lf in v_b.link_faces if lf not in faces_faixa)
    f.material_index = vz.material_index
    uv0 = next(l for l in vz.loops if l.vert is v_b)[uvl].uv.copy()
    for l in f.loops:
        l[uvl].uv = uv0
    f.smooth = True
print('FAIXA', len(faces_faixa), 'corpo', nb, 'anel', nc)
print('TOPO do traje', round(max(v.co.z for v in bm.verts), 4), 'corte', CORTE_Z)

# ---------------------------------------------------------------- costuras do rip
# Os triângulos do rip vêm soltos: cada costura são duas arestas abertas quase coladas, com pesos um pouco diferentes, que
# se abrem quando o corpo dobra. Só aresta aberta com par nas duas pontas conta como costura; os vértices dela passam a ter
# a mesma posição e os mesmos pesos. Não solda nada: UV, normais e material ficam por canto, como estavam.
COSTURA = 0.004
fixos = set(cima) | set(baixo)
abertas = [e for e in bm.edges if e.is_boundary and not (set(e.verts) & fixos)]
pos_v = list({v for e in abertas for v in e.verts})
kd_c = kdtree.KDTree(len(pos_v))
for i_, v in enumerate(pos_v):
    kd_c.insert(v.co, i_)
kd_c.balance()
abertas_de = {}
for e in abertas:
    for v in e.verts:
        abertas_de.setdefault(v, []).append(e)
pai = {v: v for v in pos_v}
def raiz(v):
    while pai[v] is not v:
        pai[v] = pai[pai[v]]; v = pai[v]
    return v
n_costuras = 0
for e in abertas:
    a_, b_ = e.verts
    if e.calc_length() < COSTURA * 2:
        continue
    for co_, j_, d_ in kd_c.find_range(a_.co, COSTURA):
        c_ = pos_v[j_]
        if c_ is a_ or c_ is b_:
            continue
        for e2 in abertas_de[c_]:
            d2 = e2.other_vert(c_)
            if e2 is not e and d2 is not a_ and d2 is not b_ and (d2.co - b_.co).length < COSTURA:
                pai[raiz(a_)] = raiz(c_); pai[raiz(b_)] = raiz(d2); n_costuras += 1
grupos_c = {}
for v in pos_v:
    grupos_c.setdefault(raiz(v), []).append(v)
grupos_c = [g_ for g_ in grupos_c.values() if len(g_) > 1]
maior = 0.0
for g_ in grupos_c:
    medio = sum((v.co for v in g_), Vector()) / len(g_)
    maior = max(maior, max((v.co - medio).length for v in g_))
    soma = {}
    for v in g_:
        for n_, x in peso_v.get(v, {}).items():
            soma[n_] = soma.get(n_, 0) + x / len(g_)
    for v in g_:
        v.co = medio.copy(); peso_v[v] = dict(soma)
print('COSTURAS', n_costuras // 2, 'arestas,', len(grupos_c), 'grupos,', sum(len(g_) for g_ in grupos_c), 'vértices; maior ajuste', round(maior * 1000, 2), 'mm')

# ---------------------------------------------------------------- objeto final, com o rig e os pesos do Indra
bm.verts.index_update()
idx_anel = {v.index: a for v, a in zip(cima, cima_d)}
pesos_idx = {v.index: peso_v.get(v, {}) for v in bm.verts}
bm.transform(HMi)
me = bpy.data.meshes.new(NOME); bm.to_mesh(me); bm.free()
ob = bpy.data.objects.new(NOME, me); bpy.context.scene.collection.objects.link(ob)
ob.parent = cabeca.parent; ob.matrix_parent_inverse = cabeca.matrix_parent_inverse.copy(); ob.matrix_basis = cabeca.matrix_basis.copy()
mats = [f'M_Traje_{PERSONAGEM}_{k + 1}' for k in range(len(SLOTS_CORPO))]
for n in mats:
    me.materials.append(bpy.data.materials.get(n) or bpy.data.materials.new(n))
usados = sorted({p.material_index for p in me.polygons}); print('SLOTS usados', usados)
for b_ in rig.data.bones:
    ob.vertex_groups.new(name=b_.name)
sem_peso = 0
for i, w in pesos_idx.items():
    if not w:
        sem_peso += 1
    for n, x in w.items():
        if x > 1e-4:
            ob.vertex_groups[n].add([i], x, 'REPLACE')
print('PESOS vértices sem peso', sem_peso)
mod = ob.modifiers.new('Indra', 'ARMATURE'); mod.object = rig
bpy.context.view_layer.update()

# normais: as do corpo encaixado; no anel, as da cabeça
ref_ob.matrix_world = Matrix.Identity(4)
dt = ob.modifiers.new('normais', 'DATA_TRANSFER'); dt.object = ref_ob
dt.use_loop_data = True; dt.data_types_loops = {'CUSTOM_NORMAL'}; dt.loop_mapping = 'POLYINTERP_NEAREST'; dt.use_object_transform = True
ob.modifiers.move(len(ob.modifiers) - 1, 0)
with bpy.context.temp_override(object=ob, active_object=ob, selected_objects=[ob]):
    bpy.ops.object.modifier_apply(modifier=dt.name)
ln = [Vector(n.vector) for n in me.corner_normals]
M3t = HM.to_3x3().transposed()
for li, l in enumerate(me.loops):
    if l.vertex_index in idx_anel:
        ln[li] = (M3t @ idx_anel[l.vertex_index]['n']).normalized()
me.normals_split_custom_set(ln)
bpy.data.objects.remove(ref_ob)

# ---------------------------------------------------------------- máscaras de pele (por cor da textura)
info_pele = {}
if FBX_ORIGEM:
    imgs = {str(k + 1): bpy.data.images.load(str(ORIGEM / (n + '.png'))) for k, n in enumerate(FBX_ORIGEM['texturas'])}
else:
    imgs = {'1': bpy.data.images['T_KakashiAnbu_1'], '2': bpy.data.images['T_KakashiAnbu_2']}
amostras = []
for k, im in imgs.items():
    W, H_ = im.size
    px = np.array(im.pixels[:], dtype=np.float32).reshape(H_, W, 4)[:, :, :3]
    sat = px[:, :, 0] - px[:, :, 2]
    # pele: clara, avermelhada e pouco saturada (nem o vermelho da tatuagem nem o cinza da roupa)
    pele = (px[:, :, 0] > 0.72) & (sat > 0.08) & (sat < 0.35) & (px[:, :, 1] > 0.55)
    amostras.append(px[pele])
    borrado = pele.astype(np.float32)
    for _ in range(1):
        borrado = (borrado + np.roll(borrado, 1, 0) + np.roll(borrado, -1, 0) + np.roll(borrado, 1, 1) + np.roll(borrado, -1, 1)) / 5
    masc = bpy.data.images.new(f'T_{PERSONAGEM}_SkinMask_{k}', W, H_, alpha=False)
    masc.colorspace_settings.name = 'Non-Color'
    rgba = np.dstack([borrado, borrado, borrado, np.ones_like(borrado)]).astype(np.float32)
    masc.pixels = rgba.ravel().tolist()
    masc.filepath_raw = str(OUT / f'T_{PERSONAGEM}_SkinMask_{k}.png'); masc.file_format = 'PNG'; masc.save()
    print('MASCARA', k, 'pixels de pele', int(pele.sum()), 'de', W * H_)
ref_srgb = np.median(np.vstack(amostras), axis=0)
(OUT / 'info.json').write_text(json.dumps({'pele_ref_srgb': [round(float(c), 4) for c in ref_srgb], 'escala_grossura': round(ESCALA, 4),
                                           'ombros_m': round(float(np.linalg.norm(T['upperarm_l'] - T['upperarm_r'])), 4),
                                           'vertices': len(me.vertices), 'faixa_faces': len(faces_faixa)}, indent=1))
print('INFO', (OUT / 'info.json').read_text().replace('\n', ' '))

# ---------------------------------------------------------------- limpeza e fotos
for o in list(novos):
    bpy.data.objects.remove(o)
if FOTOS:
    FOTOS.mkdir(parents=True, exist_ok=True)
    sc = bpy.context.scene; sc.render.engine = 'BLENDER_WORKBENCH'; sc.display.shading.color_type = 'SINGLE'
    sc.display.shading.single_color = (0.75, 0.68, 0.62); sc.render.resolution_x = sc.render.resolution_y = 520
    cd = bpy.data.cameras.new('c'); cd.type = 'ORTHO'; cam = bpy.data.objects.new('c', cd); sc.collection.objects.link(cam); sc.camera = cam
    for nome, d, alvo, escala in (('corpo_frente', (0, -1, 0), (0, 0, 0.9), 2.0), ('corpo_lado', (1, 0, 0), (0, 0, 0.9), 2.0),
                                  ('corpo_costas', (0, 1, 0), (0, 0, 0.9), 2.0), ('pesc_frente', (0, -1, 0.05), (0, 0, 1.47), 0.36),
                                  ('pesc_lado', (1, 0, 0.05), (0, 0, 1.47), 0.36), ('pesc_costas', (0, 1, 0.05), (0, 0, 1.47), 0.36)):
        d_ = Vector(d).normalized(); a_ = Vector(alvo)
        cd.ortho_scale = escala; cam.location = a_ + d_ * 3
        cam.rotation_euler = (-d_).to_track_quat('-Z', 'Y').to_euler()
        sc.render.filepath = str(FOTOS / f'{nome}.png'); bpy.ops.render.render(write_still=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(FOTOS / 'traje.blend'))
if not SEM_FBX:
    bpy.ops.object.select_all(action='DESELECT'); rig.select_set(True); ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=str(OUT / (NOME + '.fbx')), use_selection=True, object_types={'ARMATURE', 'MESH'},
                             add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False, apply_unit_scale=True,
                             axis_forward='-Z', axis_up='Y', mesh_smooth_type='FACE', path_mode='AUTO')
    print('EXPORTADO', NOME)
