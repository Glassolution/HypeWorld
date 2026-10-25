"""Mede os golpes do pack Anime Ninja: duração, acertos (picos de alcance da mão/pé à frente) e se começam/terminam
perto das bases de luta (Idle_Loop / Hand_Idle_Loop). Salva em Desktop/Hype World/Fab/anime_ninja_golpes.json."""
import json
import unreal

AL = unreal.AnimationLibrary
MC = unreal.MathLibrary
A = "/Game/ANAP/Animations/Attack/HandBlades/"
B = "/Game/ANAP/Animations/Base/Base_Anim_Armature_CustomMan_"
LIMBS = ["hand_l", "hand_r", "foot_l", "foot_r"]
FAMILIAS = ["Hand", "ChakraFist", "ChakraFistA", "ChakraKnuckleA", "ChakraKnuckleB", "CyakraKnuckley", "ChakraGloves", "BeniHien",
            "cmb05", "cmb07", "cmb09", "KickUp", "DoubleTurnKick", "JumpTurnKick", "Y", "Armhammer", "cmb01", "cmb04"]
paths = {}


def chain(anim, bone):
    if bone not in paths:
        paths[bone] = [str(b) for b in AL.find_bone_path_to_root(anim, bone)][::-1]
    return paths[bone]


def pose(anim, t):
    names = sorted({n for l in LIMBS + ["pelvis"] for n in chain(anim, l)})
    local = dict(zip(names, AL.get_bone_poses_for_time(anim, names, t, False)))
    out = {}
    for l in LIMBS + ["pelvis"]:
        x = None
        for b in chain(anim, l):
            x = local[b] if x is None else MC.compose_transforms(local[b], x)
        out[l] = x.translation
    pel = out["pelvis"]
    return {l: (out[l].x - pel.x, out[l].y - pel.y, out[l].z - pel.z) for l in LIMBS}, pel.z


def dist(p, q):
    return sum(sum((a - b) ** 2 for a, b in zip(p[l], q[l])) ** 0.5 for l in LIMBS) / len(LIMBS)


idles = {n: pose(unreal.load_asset(B + n), 0.0)[0] for n in ("Idle_Loop", "Hand_Idle_Loop")}
ar = unreal.AssetRegistryHelpers.get_asset_registry()
res = {}
for data in sorted(ar.get_assets_by_path(A[:-1], recursive=False), key=lambda d: str(d.asset_name)):
    name = str(data.asset_name).replace("HandBlades_Anim_Armature_CustomMan_", "")
    core = name.replace("RangeAttack_", "").replace("Attack_", "")
    aerial = core.startswith("Aerial_")
    fam = core.replace("Aerial_", "").split("_")[0]
    if fam not in FAMILIAS or name.endswith("_WEP"):
        continue
    anim = unreal.load_asset(str(data.package_name))
    L = anim.get_play_length()
    n = max(int(L / 0.033), 8)
    rows = []
    for i in range(n + 1):
        t = L * i / n
        p, pz = pose(anim, t)
        reach = max(p[l][1] for l in LIMBS)   # +Y = frente do boneco
        rows.append((t, reach, max(LIMBS, key=lambda l: p[l][1]), pz))
    mx = max(r[1] for r in rows)
    peaks = []
    for i in range(1, len(rows) - 1):
        if rows[i][1] >= rows[i - 1][1] and rows[i][1] >= rows[i + 1][1] and rows[i][1] >= mx * 0.85 and rows[i][1] > 55:
            if not peaks or rows[i][0] - peaks[-1][0] > 0.22:
                peaks.append((round(rows[i][0], 2), rows[i][2], round(rows[i][1])))
    p0, _ = pose(anim, 0.0)
    p1, _ = pose(anim, L)
    res[name] = dict(L=round(L, 2), aerial=aerial, fam=fam, hits=peaks,
                     start={k: round(dist(p0, v)) for k, v in idles.items()}, end={k: round(dist(p1, v)) for k, v in idles.items()},
                     pelvis_max=round(max(r[3] for r in rows)), path=str(data.package_name))
json.dump(res, open("/Users/luisfelipexavier/Desktop/Hype World/Fab/anime_ninja_golpes.json", "w"), indent=1)
print("golpes medidos:", len(res))
for k, v in res.items():
    print("%-44s %4.2fs %s ini %s fim %s acertos %s" % (k, v["L"], "AR " if v["aerial"] else "   ", v["start"]["Idle_Loop"], v["end"]["Idle_Loop"], [(h[0], h[1][:6]) for h in v["hits"]]))
