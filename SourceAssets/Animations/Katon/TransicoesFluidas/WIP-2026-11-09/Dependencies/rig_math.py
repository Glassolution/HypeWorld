"""Calcula poses no espaço do esqueleto aprovado. Executar com Blender em segundo plano."""
import json
import math
from pathlib import Path

from mathutils import Matrix, Quaternion, Vector

HERE = Path(__file__).resolve().parent
SOURCE = json.loads((HERE / "Clipes originais.json").read_text())
INITIAL = json.loads((HERE / "Postura inicial.json").read_text())
HIERARCHY = HERE / "hierarquia e pontos.json"
PARENTS = {name: value["parent"] for name, value in json.loads(HIERARCHY.read_text())["bones"].items()}
NAMES = list(SOURCE["ref"])
assert len(NAMES) == 161 and set(NAMES) == set(PARENTS)
FINGERS = ("thumb", "index", "middle", "ring", "pinky")


def decode(row):
    return {n: (Vector(p), Quaternion((q[3], *q[:3])), Vector(s)) for n, (p, q, s) in row.items()}


def encode(pose):
    return {n: [list(p), [q.x, q.y, q.z, q.w], list(s)] for n, (p, q, s) in pose.items()}


def world(local):
    result = {}
    for n in NAMES:
        p, q, s = local[n]
        par = PARENTS[n]
        if par != "None":
            pp, pq, ps = result[par]
            p, q, s = pp + pq @ (ps * p), pq @ q, ps * s
        result[n] = (p, q, s)
    return result


def set_rotation(local, n, desired):
    p, _, s = local[n]
    parent = world(local)[PARENTS[n]][1]
    local[n] = (p, (parent.inverted() @ desired).normalized(), s)


def aim(local, n, child, direction):
    current = world(local)
    pos, rot, _ = current[n]
    old = current[child][0] - pos
    if old.length > 1e-5:
        set_rotation(local, n, old.rotation_difference(direction) @ rot)


def frame(longitudinal, dorsal):
    x = longitudinal.normalized()
    y = dorsal.cross(x).normalized()
    return Matrix((x, y, x.cross(y))).transposed().to_quaternion()


def solve_arm(local, side, wrist, pole):
    shoulder, elbow, hand = [f"{n}_{side}" for n in ("upperarm", "lowerarm", "hand")]
    g = world(local)
    a, b, c = [g[n][0] for n in (shoulder, elbow, hand)]
    upper, lower = (b - a).length, (c - b).length
    delta = wrist - a
    d = min(max(delta.length, abs(upper - lower) + .01), upper + lower - .01)
    axis = delta.normalized()
    along = (upper ** 2 - lower ** 2 + d ** 2) / (2 * d)
    side_axis = (pole - a) - axis * (pole - a).dot(axis)
    e = a + axis * along + side_axis.normalized() * math.sqrt(max(0, upper ** 2 - along ** 2))
    aim(local, shoulder, elbow, e - a)
    aim(local, elbow, hand, wrist - e)


def tiger():
    local = decode(SOURCE["seal"]["frames"][-1])
    bind = decode(SOURCE["ref"])
    rest = world(bind)
    center = world(local)["spine_04"][0]
    for side, sign in (("l", 1), ("r", -1)):
        hand = f"hand_{side}"
        wrist = center + Vector((sign * 3.05, 22.3, -2.1))
        pole = center + Vector((sign * 34, 8, -2))
        solve_arm(local, side, wrist, pole)
        # Build the palm orientation from anatomical landmarks in the bind pose.
        pos, rotation, _ = rest[hand]
        longitudinal = rest[f"middle_01_{side}"][0] - pos
        across = rest[f"index_01_{side}"][0] - rest[f"pinky_01_{side}"][0]
        dorsal = longitudinal.cross(across) * sign
        palm = frame(Vector((-sign * .15, .07, 1)), Vector((sign, .05, 0)))
        set_rotation(local, hand, palm @ frame(longitudinal, dorsal).inverted() @ rotation)
        for finger in FINGERS:
            for index in range(1, 4):
                n = f"{finger}_{index:02d}_{side}"
                p, _, scale = local[n]
                local[n] = (p, bind[n][1], scale)
            if finger != "thumb":
                n = f"{finger}_metacarpal_{side}"
                p, _, scale = local[n]
                local[n] = (p, bind[n][1], scale)
        # The two straight index fingers converge. The other fingers curl across
        # the contact plane at staggered depths, leaving the thumbs tucked below.
        g = world(local)
        idx = g[f"index_01_{side}"][0]
        tip = Vector((center.x + sign * .54, idx.y + .7, idx.z + 8.0))
        index_direction = (tip - idx).normalized()
        for finger in FINGERS:
            n1, n2, n3 = [f"{finger}_{i:02d}_{side}" for i in (1, 2, 3)]
            if finger == "index":
                dirs = (index_direction, index_direction, index_direction)
            elif finger == "thumb":
                dirs = (Vector((-sign * .57, -.55, .55)), Vector((-sign * .35, -.1, .9)), Vector((0, .20, .85)))
            else:
                stagger = {"middle": .18, "ring": .02, "pinky": -.13}[finger]
                dirs = (Vector((-sign * .70, stagger, .67)), Vector((-sign * .88, stagger, -.18)), Vector((-sign * .24, stagger, -.91)))
            aim(local, n1, n2, dirs[0])
            aim(local, n2, n3, dirs[1])
            # There is no fingertip bone. The terminal link follows the preceding
            # link in the bind pose, so its direction can be derived in local space.
            g = world(local)
            tip_axis = rest[n3][1].inverted() @ (rest[n3][0] - rest[n2][0])
            current = g[n3][1] @ tip_axis
            set_rotation(local, n3, current.rotation_difference(dirs[2]) @ g[n3][1])
    return local


def smooth(x):
    x = min(1., max(0., x))
    return x * x * (3. - 2. * x)


def blend(a, b, weight):
    return {n: (a[n][0].lerp(b[n][0], weight), a[n][1].slerp(b[n][1], weight),
                a[n][2].lerp(b[n][2], weight)) for n in NAMES}


def sample(frames, time, length):
    k = min(len(frames)-1, max(0, time / length * (len(frames)-1)))
    lo = int(k)
    return blend(frames[lo], frames[min(lo+1,len(frames)-1)], k-lo)


def build():
    neutral = decode(INITIAL)
    # The evaluated idle can contain root motion compensation. Keep the authored
    # root track so the montage follows the existing gameplay rotation.
    neutral["root"] = decode(SOURCE["seal"]["frames"][0])["root"]
    pose = tiger()
    ref = world(decode(SOURCE["ref"]))
    wrist, wrist_rotation, _ = ref["hand_r"]
    long = ref["middle_01_r"][0] - wrist
    across = ref["index_01_r"][0] - ref["pinky_01_r"][0]
    right_palm = frame(long, -long.cross(across)).inverted() @ wrist_rotation
    result = {"tiger_pose": encode(pose)}
    for tag, count in (("seal", 30), ("breath", 72), ("orbit", 30)):
        raw = [decode(f) for f in SOURCE[tag]["frames"]]
        duration = SOURCE[tag]["length"]
        frames = []
        for i in range(count+1):
            t = duration * i / count
            original = sample(raw, t, duration)
            if tag == "seal":
                authored = blend(neutral, pose, smooth(t/.40))
            elif tag == "orbit":
                # Keep the release and its original open-arm pose at 0.4375 s.
                authored = blend(pose, original, smooth(t/.35))
                authored = blend(authored, neutral, smooth((t-.46)/(.625-.46)))
            else:
                # Open from the shared seal. Keep the mouth-side hand moving
                # towards the breath and drop the support hand towards the ribs.
                authored = blend(pose, original, smooth(t/.28))
                g = world(authored)
                center = g["spine_04"][0]
                for side, sign in (("l", 1), ("r", -1)):
                    hand = f"hand_{side}"
                    hand_pos, hand_rotation, _ = g[hand]
                    if side == "l":
                        goal = center + Vector((14., 17., -6.))
                        weight = smooth(t/.21) * (1.-smooth((t-.43)/.45))
                    else:
                        head = g["head"][0]
                        goal = head + Vector((-8.5, 13., -11.))
                        breath_hand = frame(Vector((.72, -.23, .62)), Vector((-.10, 1., 0.))) @ right_palm
                        weight = smooth(t/.21) * (1.-smooth((t-.43)/.32))
                    old_elbow = g[f"lowerarm_{side}"][0]
                    low_elbow = center + Vector((sign * 32., 7., -15.))
                    solve_arm(authored, side, hand_pos.lerp(goal,weight), old_elbow.lerp(low_elbow, weight))
                    if side == "r":
                        hand_rotation = hand_rotation.slerp(breath_hand, weight)
                    set_rotation(authored, hand, hand_rotation)
                authored = blend(authored,neutral,smooth((t-1.05)/.45))
            if (tag == "seal" and i == count) or (tag != "seal" and i == 0):
                authored = pose
            frames.append(encode(authored))
        result[tag] = frames
    (HERE / "Clipes revisados.json").write_text(json.dumps(result,separators=(",",":")))
    print("KATON_POSES_READY", {k:len(v) for k,v in result.items()})
