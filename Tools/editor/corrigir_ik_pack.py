"""Corrige os ossos guia (ik_foot_*, ik_hand_*) das animações do pack Anime Ninja: no pack eles ficam parados embaixo do
corpo, e o Anim Blueprint (Foot IK da Epic) cola os pés neles -> pernas fechadas/cruzadas. Aqui eles passam a seguir
os pés e as mãos de verdade, quadro a quadro (como nas animações da Epic).
Variáveis antes do código: PASTA (padrão /Game/ANAP/Animations) e SO (lista de caminhos; vazio = todas da pasta).
"""
import unreal

AL = unreal.AnimationLibrary
MC = unreal.MathLibrary
PASTA = globals().get("PASTA", "/Game/ANAP/Animations")
SO = globals().get("SO", [])
skel_mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")


def chain(anim, bone):
    return [str(b) for b in AL.find_bone_path_to_root(anim, bone)][::-1]   # raiz -> osso


def fix(anim):
    guides = {"ik_foot_l": "foot_l", "ik_foot_r": "foot_r", "ik_hand_gun": "hand_r", "ik_hand_l": "hand_l", "ik_hand_r": "hand_r"}
    paths = {b: chain(anim, b) for b in list(guides) + list(guides.values())}
    names = sorted({n for p in paths.values() for n in p})
    nkeys = AL.get_num_frames(anim) + 1
    keys = {g: ([], [], []) for g in guides}
    for f in range(nkeys):
        local = dict(zip(names, AL.get_bone_poses_for_frame(anim, names, min(f, nkeys - 1), False)))
        cs = {}
        def comp(b):
            if b not in cs:
                p = paths_parent.get(b)
                cs[b] = local[b] if p is None else MC.compose_transforms(local[b], comp(p))
            return cs[b]
        for g, target in guides.items():
            parent = paths[g][-2]
            want = MC.compose_transforms(comp(target), MC.invert_transform(comp(parent)))   # local = CS_alvo * CS_pai^-1
            keys[g][0].append(want.translation)
            keys[g][1].append(want.rotation)
            keys[g][2].append(want.scale3d)
    ctrl = anim.get_editor_property("controller")
    ctrl.open_bracket(unreal.Text("Corrigir ossos guia"), False)
    for g, (pos, rot, scl) in keys.items():
        ctrl.add_bone_curve(g, False)
        ctrl.set_bone_track_keys(g, pos, rot, scl, False)
    ctrl.close_bracket(False)


paths_parent = {}
ar = unreal.AssetRegistryHelpers.get_asset_registry()
lista = SO or sorted(str(a.package_name) for a in ar.get_assets_by_path(PASTA, recursive=True) if str(a.asset_class_path.asset_name) == "AnimSequence")
feitas = 0
for i, path in enumerate(lista):
    anim = unreal.load_asset(path)
    if not paths_parent:
        for b in ["ik_foot_l", "ik_foot_r", "ik_hand_gun", "ik_hand_l", "ik_hand_r", "foot_l", "foot_r", "hand_l", "hand_r"]:
            p = chain(anim, b)
            for a_, b_ in zip(p[:-1], p[1:]):
                paths_parent[b_] = a_
        paths_parent[chain(anim, "foot_l")[0]] = None
    fix(anim)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    feitas += 1
    if i % 40 == 0:
        print("...", i, path.split("/")[-1])
print("corrigidas:", feitas)
